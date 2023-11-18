//
//  renderer.c
//  C-ray
//
//  Created by Valtteri Koskivuori on 19/02/2017.
//  Copyright © 2017-2022 Valtteri Koskivuori. All rights reserved.
//

#include "../utils/hashtable.h"
#include "../includes.h"

#include "../datatypes/image/imagefile.h"
#include "renderer.h"
#include "../datatypes/camera.h"
#include "../datatypes/scene.h"
#include "pathtrace.h"
#include "../utils/logging.h"
#include "../datatypes/tile.h"
#include "../utils/timer.h"
#include "../datatypes/image/texture.h"
#include "../datatypes/mesh.h"
#include "../datatypes/sphere.h"
#include "../utils/platform/thread.h"
#include "../utils/platform/mutex.h"
#include "samplers/sampler.h"
#include "../utils/platform/capabilities.h"
#include "../utils/platform/signal.h"
#include "../utils/protocol/server.h"
#include "../utils/string.h"
#include "../accelerators/bvh.h"

//Main thread loop speeds
#define paused_msec 100
#define active_msec  16

static bool g_aborted = false;

void sigHandler(int sig) {
	if (sig == 2) { //SIGINT
		logr(plain, "\n");
		logr(info, "Received ^C, aborting render without saving\n");
		g_aborted = true;
	}
}

static void printSceneStats(struct world *scene, unsigned long long ms) {
	logr(info, "Scene construction completed in ");
	printSmartTime(ms);
	uint64_t polys = 0;
	uint64_t vertices = 0;
	uint64_t normals = 0;
	for (size_t i = 0; i < scene->instances.count; ++i) {
		if (isMesh(&scene->instances.items[i])) {
			const struct mesh *mesh = &scene->meshes.items[scene->instances.items[i].object_idx];
			polys += mesh->polygons.count;
			vertices += mesh->vbuf->vertices.count;
			normals += mesh->vbuf->normals.count;
		}
	}
	logr(plain, "\n");
	logr(info, "Totals: %liV, %liN, %zuI, %liP, %zuS, %zuM\n",
		   vertices,
		   normals,
		   scene->instances.count,
		   polys,
		   scene->spheres.count,
		   scene->meshes.count);
}

void *renderThread(void *arg);
void *renderThreadInteractive(void *arg);

//FIXME: Statistics computation is a gigantic mess. It will also break in the case
//where a worker node disconnects during a render, so maybe fix that next.
void update_cb_info(struct renderer *r, struct cr_renderer_cb_info *i) {
	static uint64_t ctr = 1;
	static uint64_t avg_per_sample_us = 0;
	static uint64_t avg_tile_pass_us = 0;
	i->user_data = r->state.cb.user_data;
	// Notice: Casting away const here
	memcpy((struct cr_tile *)i->tiles, r->state.tiles.items, sizeof(*i->tiles) * i->tiles_count);
	if (!r->state.workers) return;
	//Gather and maintain this average constantly.
	size_t total_thread_count = r->prefs.threads + (int)r->state.clients.count;
	size_t remote_threads = 0;
	for (size_t i = 0; i < r->state.clients.count; ++i) {
		remote_threads += r->state.clients.items[i].available_threads;
	}
	if (!r->state.workers[0].paused) { // FIXME: Use renderer state instead
		for (size_t t = 0; t < total_thread_count; ++t) {
			avg_per_sample_us += r->state.workers[t].avg_per_sample_us; // FIXME: Not updated from remote nodes
		}
		avg_tile_pass_us += avg_per_sample_us / total_thread_count;
		avg_tile_pass_us /= ctr++;
	}
	double avg_per_ray_us = (double)avg_tile_pass_us / (double)(r->prefs.tileHeight * r->prefs.tileWidth);
	uint64_t completed_samples = 0;
	for (size_t t = 0; t < total_thread_count; ++t) {
		completed_samples += r->state.workers[t].totalSamples;
	}
	uint64_t remainingTileSamples = (r->state.tiles.count * r->prefs.sampleCount) - completed_samples;
	uint64_t eta_ms_till_done = (avg_tile_pass_us * remainingTileSamples) / 1000;
	eta_ms_till_done /= (r->prefs.threads + remote_threads);
	uint64_t sps = (1000000 / avg_per_ray_us) * (r->prefs.threads + remote_threads);

	i->paused = r->state.workers[0].paused;
	i->avg_per_ray_us = avg_per_ray_us;
	i->samples_per_sec = sps;
	i->eta_ms = eta_ms_till_done;
	i->completion = r->prefs.iterative ?
		((double)r->state.finishedPasses / (double)r->prefs.sampleCount) :
		((double)r->state.finishedTileCount / (double)r->state.tiles.count);

}

/// @todo Use defaultSettings state struct for this.
/// @todo Clean this up, it's ugly.
struct texture *renderFrame(struct renderer *r) {
	//Check for CTRL-C
	if (registerHandler(sigint, sigHandler)) {
		logr(warning, "Unable to catch SIGINT\n");
	}

	struct camera camera = r->scene->cameras.items[r->prefs.selected_camera];
	if (r->prefs.override_width && r->prefs.override_height) {
		camera.width = r->prefs.override_width ? (int)r->prefs.override_width : camera.width;
		camera.height = r->prefs.override_height ? (int)r->prefs.override_height : camera.height;
		cam_recompute_optics(&camera);
	}

	logr(info, "Starting c-ray renderer for frame %zu\n", r->prefs.imgCount);
	
	// Verify we have at least a single thread rendering.
	if (r->state.clients.count == 0 && r->prefs.threads < 1) {
		logr(warning, "No network render workers, setting thread count to 1\n");
		r->prefs.threads = 1;
	}
	
	bool threadsReduced = (size_t)getSysCores() > r->prefs.threads;
	
	logr(info, "Rendering at %s%i%s x %s%i%s\n", KWHT, camera.width, KNRM, KWHT, camera.height, KNRM);
	logr(info, "Rendering %s%zu%s samples with %s%zu%s bounces.\n", KBLU, r->prefs.sampleCount, KNRM, KGRN, r->prefs.bounces, KNRM);
	logr(info, "Rendering with %s%zu%s%s local thread%s.\n",
		 KRED,
		 r->prefs.fromSystem && !threadsReduced ? r->prefs.threads - 2 : r->prefs.threads,
		 r->prefs.fromSystem && !threadsReduced ? "+2" : "",
		 KNRM,
		 PLURAL(r->prefs.threads));
	
	//Quantize image into renderTiles
	tile_quantize(&r->state.tiles,
					camera.width,
					camera.height,
					r->prefs.tileWidth,
					r->prefs.tileHeight,
					r->prefs.tileOrder);

	// Do some pre-render preparations
	// Compute BVH acceleration structures for all meshes in the scene
	compute_accels(r->scene->meshes);

	// And then compute a single top-level BVH that contains all the objects
	logr(info, "Computing top-level BVH: ");
	struct timeval timer = {0};
	timer_start(&timer);
	r->scene->topLevel = build_top_level_bvh(r->scene->instances);
	printSmartTime(timer_get_ms(timer));
	logr(plain, "\n");

	printSceneStats(r->scene, timer_get_ms(timer));

	for (size_t i = 0; i < r->state.tiles.count; ++i)
		r->state.tiles.items[i].total_samples = r->prefs.sampleCount;

	//Print a useful warning to user if the defined tile size results in less renderThreads
	if (r->state.tiles.count < r->prefs.threads) {
		logr(warning, "WARNING: Rendering with a less than optimal thread count due to large tile size!\n");
		logr(warning, "Reducing thread count from %zu to %zu\n", r->prefs.threads, r->state.tiles.count);
		r->prefs.threads = r->state.tiles.count;
	}

	struct texture *output = newTexture(char_p, camera.width, camera.height, 3);
	struct cr_tile *info_tiles = calloc(r->state.tiles.count, sizeof(*info_tiles));
	struct cr_renderer_cb_info cb_info = {
		.tiles = info_tiles,
		.tiles_count = r->state.tiles.count
	};
	cb_info.fb = output;
	if (r->state.cb.cr_renderer_on_start) {
		update_cb_info(r, &cb_info);
		r->state.cb.cr_renderer_on_start(&cb_info);
	}

	logr(info, "Pathtracing%s...\n", r->prefs.iterative ? " iteratively" : "");
	
	r->state.rendering = true;
	r->state.render_aborted = false;
	r->state.saveImage = true; // Set to false if user presses X
	
	size_t remoteThreads = 0;
	for (size_t i = 0; i < r->state.clients.count; ++i) {
		remoteThreads += r->state.clients.items[i].available_threads;
	}
	
	if (r->state.clients.count) logr(info, "Using %lu render worker%s totaling %lu thread%s.\n", r->state.clients.count, PLURAL(r->state.clients.count), remoteThreads, PLURAL(remoteThreads));
	
	// Select the appropriate renderer type for local use
	void *(*localRenderThread)(void *) = renderThread;
	// Iterative mode is incompatible with network rendering at the moment
	if (r->prefs.iterative && !r->state.clients.count) localRenderThread = renderThreadInteractive;
	
	// Local render threads + one thread for every client
	size_t total_thread_count = r->prefs.threads + (int)r->state.clients.count;
	r->state.workers = calloc(total_thread_count, sizeof(*r->state.workers));

	//Allocate memory for render buffer
	//Render buffer is used to store accurate color values for the renderers' internal use
	r->state.renderBuffer = newTexture(float_p, camera.width, camera.height, 3);
	
	//Create & boot workers (Nonblocking)
	for (int t = 0; t < (int)total_thread_count; ++t) {
		r->state.workers[t] = (struct worker){
			.client = t > (int)r->prefs.threads - 1 ? &r->state.clients.items[t - r->prefs.threads] : NULL,
			.thread_complete = false,
			.renderer = r,
			.output = output,
			.cam = &camera,
			.thread = (struct cr_thread){
				.thread_fn = t > (int)r->prefs.threads - 1 ? client_connection_thread : localRenderThread,
				.user_data = &r->state.workers[t]
			}
		};
		if (thread_start(&r->state.workers[t].thread))
			logr(error, "Failed to start worker %d\n", t);
	}

	//Start main thread loop to handle renderer feedback and state management
	while (r->state.rendering) {
		if (g_aborted) {
			r->state.saveImage = false;
			r->state.render_aborted = true;
		}
		
		if (r->state.cb.cr_renderer_status) {
			update_cb_info(r, &cb_info);
			r->state.cb.cr_renderer_status(&cb_info);
		}

		size_t inactive = 0;
		for (size_t t = 0; t < total_thread_count; ++t) {
			if (r->state.workers[t].thread_complete) inactive++;
		}
		if (r->state.render_aborted || inactive == total_thread_count)
			r->state.rendering = false;
		timer_sleep_ms(r->state.workers[0].paused ? paused_msec : active_msec);
	}
	
	//Make sure render threads are terminated before continuing (This blocks)
	for (size_t t = 0; t < total_thread_count; ++t) {
		thread_wait(&r->state.workers[t].thread);
	}
	if (r->state.cb.cr_renderer_on_stop) {
		update_cb_info(r, &cb_info);
		r->state.cb.cr_renderer_on_stop(&cb_info);
	}
	if (info_tiles) free(info_tiles);
	return output;
}

// An interactive render thread that progressively
// renders samples up to a limit
void *renderThreadInteractive(void *arg) {
	block_signals();
	struct worker *threadState = (struct worker*)thread_user_data(arg);
	struct renderer *r = threadState->renderer;
	struct texture *image = threadState->output;
	sampler *sampler = newSampler();

	struct camera *cam = threadState->cam;
	
	//First time setup for each thread
	struct render_tile *tile = tile_next_interactive(r);
	threadState->currentTile = tile;
	
	struct timeval timer = {0};
	
	threadState->completedSamples = 1;
	
	while (tile && r->state.rendering) {
		long total_us = 0;

		timer_start(&timer);
		for (int y = tile->end.y - 1; y > tile->begin.y - 1; --y) {
			for (int x = tile->begin.x; x < tile->end.x; ++x) {
				if (r->state.render_aborted) goto exit;
				uint32_t pixIdx = (uint32_t)(y * image->width + x);
				//FIXME: This does not converge to the same result as with regular renderThread.
				//I assume that's because we'd have to init the sampler differently when we render all
				//the tiles in one go per sample, instead of the other way around.
				initSampler(sampler, SAMPLING_STRATEGY, r->state.finishedPasses, r->prefs.sampleCount, pixIdx);
				
				struct color output = textureGetPixel(r->state.renderBuffer, x, y, false);
				struct color sample = path_trace(cam_get_ray(cam, x, y, sampler), r->scene, r->prefs.bounces, sampler);

				nan_clamp(&sample, &output);
				
				//And process the running average
				output = colorCoef((float)(r->state.finishedPasses - 1), output);
				output = colorAdd(output, sample);
				float t = 1.0f / r->state.finishedPasses;
				output = colorCoef(t, output);
				
				//Store internal render buffer (float precision)
				setPixel(r->state.renderBuffer, output, x, y);
				
				//Gamma correction
				output = colorToSRGB(output);
				
				//And store the image data
				setPixel(image, output, x, y);
			}
		}
		//For performance metrics
		total_us += timer_get_us(timer);
		threadState->totalSamples++;
		threadState->completedSamples++;
		//Pause rendering when bool is set
		while (threadState->paused && !r->state.render_aborted) {
			timer_sleep_ms(100);
		}
		threadState->avg_per_sample_us = total_us / r->state.finishedPasses;
		
		//Tile has finished rendering, get a new one and start rendering it.
		tile->state = finished;
		threadState->currentTile = NULL;
		threadState->completedSamples = r->state.finishedPasses;
		tile = tile_next_interactive(r);
		threadState->currentTile = tile;
	}
exit:
	destroySampler(sampler);
	//No more tiles to render, exit thread. (render done)
	threadState->thread_complete = true;
	threadState->currentTile = NULL;
	return 0;
}

/**
 A render thread
 
 @param arg Thread information (see threadInfo struct)
 @return Exits when thread is done
 */
void *renderThread(void *arg) {
	block_signals();
	struct worker *threadState = (struct worker*)thread_user_data(arg);
	struct renderer *r = threadState->renderer;
	struct texture *image = threadState->output;
	sampler *sampler = newSampler();

	struct camera *cam = threadState->cam;

	//First time setup for each thread
	struct render_tile *tile = tile_next(r);
	threadState->currentTile = tile;
	
	struct timeval timer = {0};
	threadState->completedSamples = 1;
	
	while (tile && r->state.rendering) {
		long total_us = 0;
		long samples = 0;
		
		while (threadState->completedSamples < r->prefs.sampleCount + 1 && r->state.rendering) {
			timer_start(&timer);
			for (int y = tile->end.y - 1; y > tile->begin.y - 1; --y) {
				for (int x = tile->begin.x; x < tile->end.x; ++x) {
					if (r->state.render_aborted) goto exit;
					uint32_t pixIdx = (uint32_t)(y * image->width + x);
					initSampler(sampler, SAMPLING_STRATEGY, threadState->completedSamples - 1, r->prefs.sampleCount, pixIdx);
					
					struct color output = textureGetPixel(r->state.renderBuffer, x, y, false);
					struct color sample = path_trace(cam_get_ray(cam, x, y, sampler), r->scene, r->prefs.bounces, sampler);
					
					// Clamp out fireflies - This is probably not a good way to do that.
					nan_clamp(&sample, &output);

					//And process the running average
					output = colorCoef((float)(threadState->completedSamples - 1), output);
					output = colorAdd(output, sample);
					float t = 1.0f / threadState->completedSamples;
					output = colorCoef(t, output);
					
					//Store internal render buffer (float precision)
					setPixel(r->state.renderBuffer, output, x, y);
					
					//Gamma correction
					output = colorToSRGB(output);
					
					//And store the image data
					setPixel(image, output, x, y);
				}
			}
			//For performance metrics
			samples++;
			total_us += timer_get_us(timer);
			threadState->totalSamples++;
			threadState->completedSamples++;
			tile->completed_samples++;
			//Pause rendering when bool is set
			while (threadState->paused && !r->state.render_aborted) {
				timer_sleep_ms(100);
			}
			threadState->avg_per_sample_us = total_us / samples;
		}
		//Tile has finished rendering, get a new one and start rendering it.
		tile->state = finished;
		threadState->currentTile = NULL;
		threadState->completedSamples = 1;
		tile = tile_next(r);
		threadState->currentTile = tile;
	}
exit:
	destroySampler(sampler);
	//No more tiles to render, exit thread. (render done)
	threadState->thread_complete = true;
	threadState->currentTile = NULL;
	return 0;
}

static struct prefs defaults() {
	return (struct prefs){
			.tileOrder = ro_from_middle,
			.threads = getSysCores() + 2,
			.fromSystem = true,
			.sampleCount = 25,
			.bounces = 20,
			.tileWidth = 32,
			.tileHeight = 32,
			.imgFilePath = stringCopy("./"),
			.assetPath = stringCopy("./"),
			.imgFileName = stringCopy("rendered"),
			.imgCount = 0,
			.imgType = png,
	};
}

struct renderer *renderer_new() {
	struct renderer *r = calloc(1, sizeof(*r));
	r->prefs = defaults();
	r->state.finishedPasses = 1;
	
	r->state.tileMutex = mutex_create();

	r->scene = calloc(1, sizeof(*r->scene));
	r->scene->storage.node_pool = newBlock(NULL, 1024);
	r->scene->storage.node_table = newHashtable(compareNodes, &r->scene->storage.node_pool);
	return r;
}
	
void renderer_destroy(struct renderer *r) {
	if (!r) return;
	scene_destroy(r->scene);
	if (r->state.renderBuffer) destroyTexture(r->state.renderBuffer);
	render_tile_arr_free(&r->state.tiles);
	free(r->state.workers);
	free(r->state.tileMutex);
	render_client_arr_free(&r->state.clients);
	if (r->state.file_cache) {
		cache_destroy(r->state.file_cache);
		free(r->state.file_cache);
	}
	free(r->prefs.imgFileName);
	free(r->prefs.imgFilePath);
	free(r->prefs.assetPath);
	if (r->prefs.node_list) free(r->prefs.node_list);
	free(r);
}
