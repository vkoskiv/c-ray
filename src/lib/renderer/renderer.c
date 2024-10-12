//
//  renderer.c
//  c-ray
//
//  Created by Valtteri Koskivuori on 19/02/2017.
//  Copyright © 2017-2025 Valtteri Koskivuori. All rights reserved.
//

#include "../../includes.h"

#include "renderer.h"
#include "pathtrace.h"
#include <common/hashtable.h>
#include <common/logging.h>
#include <common/timer.h>
#include <common/texture.h>
#include <common/platform/thread.h>
#include <common/platform/mutex.h>
#include <common/platform/capabilities.h>
#include <common/platform/signal.h>
#include <common/platform/thread_pool.h>
#include <common/cr_string.h>
#include <datatypes/mesh.h>
#include <datatypes/camera.h>
#include <datatypes/scene.h>
#include <datatypes/tile.h>
#include <datatypes/sphere.h>
#include <protocol/server.h>
#include <accelerators/bvh.h>
#include "samplers/sampler.h"

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

static void print_stats(const struct world *scene) {
	uint64_t polys = 0;
	uint64_t vertices = 0;
	uint64_t normals = 0;
	for (size_t i = 0; i < scene->instances.count; ++i) {
		if (isMesh(&scene->instances.items[i])) {
			const struct mesh *mesh = &scene->meshes.items[scene->instances.items[i].object_idx];
			polys += mesh->polygons.count;
			vertices += mesh->vbuf.vertices.count;
			normals += mesh->vbuf.normals.count;
		}
	}
	logr(info, "Totals: %liV, %liN, %zuI, %liP, %zuS, %zuM\n",
		   vertices,
		   normals,
		   scene->instances.count,
		   polys,
		   scene->spheres.count,
		   scene->meshes.count);
}

void *render_thread(void *arg);
void *render_thread_interactive(void *arg);

//FIXME: Statistics computation is a gigantic mess. It will also break in the case
//where a worker node disconnects during a render, so maybe fix that next.
void update_cb_info(struct renderer *r, struct tile_set *set, struct cr_renderer_cb_info *i) {
	static uint64_t ctr = 1;
	static uint64_t avg_per_sample_us = 0;
	static uint64_t avg_tile_pass_us = 0;
	// Notice: Casting away const here
	memcpy((struct cr_tile *)i->tiles, set->tiles.items, sizeof(*i->tiles) * i->tiles_count);
	if (!r->state.workers.count) return;
	//Gather and maintain this average constantly.
	size_t remote_threads = 0;
	for (size_t i = 0; i < r->state.clients.count; ++i) {
		remote_threads += r->state.clients.items[i].available_threads;
	}
	if (!r->state.workers.items[0].paused) { // FIXME: Use renderer state instead
		for (size_t t = 0; t < r->state.workers.count; ++t) {
			avg_per_sample_us += r->state.workers.items[t].avg_per_sample_us; // FIXME: Not updated from remote nodes
		}
		avg_tile_pass_us += avg_per_sample_us / r->state.workers.count;
		avg_tile_pass_us /= ctr++;
	}
	double avg_per_ray_us = (double)avg_tile_pass_us / (double)(r->prefs.tileHeight * r->prefs.tileWidth);
	uint64_t completed_samples = 0;
	for (size_t t = 0; t < r->state.workers.count; ++t) {
		completed_samples += r->state.workers.items[t].totalSamples;
	}
	uint64_t remainingTileSamples = (set->tiles.count * r->prefs.sampleCount) - completed_samples;
	uint64_t eta_ms_till_done = (avg_tile_pass_us * remainingTileSamples) / 1000;
	eta_ms_till_done /= (r->prefs.threads + remote_threads);
	uint64_t sps = (1000000 / avg_per_ray_us) * (r->prefs.threads + remote_threads);

	i->paused = r->state.workers.items[0].paused;
	i->avg_per_ray_us = avg_per_ray_us;
	i->samples_per_sec = sps;
	i->eta_ms = eta_ms_till_done;
	i->completion = r->prefs.iterative ?
		((double)r->state.finishedPasses / (double)r->prefs.sampleCount) :
		((double)set->finished / (double)set->tiles.count);

}

static void *thread_stub(void *arg) {
	struct renderer *r = arg;
	renderer_render(r);
	return NULL;
}

// Nonblocking function to make python happy, just shove the normal loop in a
// background thread.
void renderer_start_interactive(struct renderer *r) {
	thread_start(&(struct cr_thread){
		.thread_fn = thread_stub,
		.user_data = r
	});
}

void update_toplevel_bvh(struct world *s) {
	if (!s->top_level_dirty && s->topLevel) return;
	struct bvh *new = build_top_level_bvh(s->instances);
	//!//!//!//!//!//!//!//!//!//!//!//!
	thread_rwlock_wrlock(&s->bvh_lock);
	struct bvh *old = s->topLevel;
	s->topLevel = new;
	thread_rwlock_unlock(&s->bvh_lock);
	//!//!//!//!//!//!//!//!//!//!//!//!
	destroy_bvh(old);
	s->top_level_dirty = false;
}

// TODO: Clean this up, it's ugly.
void renderer_render(struct renderer *r) {
	//Check for CTRL-C
	// TODO: Move signal to driver
	if (registerHandler(sigint, sigHandler)) {
		logr(warning, "Unable to catch SIGINT\n");
	}

	struct camera *camera = &r->scene->cameras.items[r->prefs.selected_camera];
	if (r->prefs.override_width && r->prefs.override_height) {
		camera->width = r->prefs.override_width ? (int)r->prefs.override_width : camera->width;
		camera->height = r->prefs.override_height ? (int)r->prefs.override_height : camera->height;
		cam_recompute_optics(camera);
	}

	// Verify we have at least a single thread rendering.
	if (r->state.clients.count == 0 && r->prefs.threads < 1) {
		logr(warning, "No network render workers, setting thread count to 1\n");
		r->prefs.threads = 1;
	}

	if (!r->scene->background) {
		r->scene->background = newBackground(&r->scene->storage, NULL, NULL, NULL, r->scene->use_blender_coordinates);
	}
	
	struct tile_set set = tile_quantize(camera->width, camera->height, r->prefs.tileWidth, r->prefs.tileHeight, r->prefs.tileOrder);
	r->state.current_set = &set;

	for (size_t i = 0; i < r->scene->shader_buffers.count; ++i) {
		if (!r->scene->shader_buffers.items[i].bsdfs.count) {
			logr(warning, "bsdf buffer %zu is empty, patching in placeholder\n", i);
			bsdf_node_ptr_arr_add(&r->scene->shader_buffers.items[i].bsdfs, build_bsdf_node((struct cr_scene *)r->scene, NULL));
		}
	}
	// Bind object buffers to instances
	for (size_t i = 0; i < r->scene->instances.count; ++i) {
		struct instance *inst = &r->scene->instances.items[i];
		inst->bbuf = &r->scene->shader_buffers.items[inst->bbuf_idx];
	}
	
	// Ensure BVHs are up to date
	logr(debug, "Waiting for BVH thread pool\n");
	thread_pool_wait(r->scene->bvh_builder);
	logr(debug, "Continuing\n");

	// And compute an initial top-level BVH.
	update_toplevel_bvh(r->scene);

	print_stats(r->scene);

	for (size_t i = 0; i < set.tiles.count; ++i)
		set.tiles.items[i].total_samples = r->prefs.sampleCount;

	//Print a useful warning to user if the defined tile size results in less renderThreads
	if (set.tiles.count < r->prefs.threads) {
		logr(warning, "WARNING: Rendering with a less than optimal thread count due to large tile size!\n");
		logr(warning, "Reducing thread count from %zu to %zu\n", r->prefs.threads, set.tiles.count);
		r->prefs.threads = set.tiles.count;
	}

	// Render buffer is used to store accurate color values for the renderers' internal use
	if (!r->state.result_buf) {
		// Allocate
		r->state.result_buf = newTexture(float_p, camera->width, camera->height, 4);
	} else if (r->state.result_buf->width != (size_t)camera->width || r->state.result_buf->height != (size_t)camera->height) {
		// Resize
		if (r->state.result_buf) destroyTexture(r->state.result_buf);
		r->state.result_buf = newTexture(float_p, camera->width, camera->height, 4);
	} else {
		// Clear
		tex_clear(r->state.result_buf);
	}

	struct texture **result = &r->state.result_buf;

	struct cr_tile *info_tiles = calloc(set.tiles.count, sizeof(*info_tiles));
	struct cr_renderer_cb_info cb_info = {
		.tiles = info_tiles,
		.tiles_count = set.tiles.count,
		.fb = (const struct cr_bitmap **)result,
	};
	
	struct callback start = r->state.callbacks[cr_cb_on_start];
	if (start.fn) {
		update_cb_info(r, &set, &cb_info);
		start.fn(&cb_info, start.user_data);
	}

	logr(info, "Pathtracing%s...\n", r->prefs.iterative ? " iteratively" : "");
	
	r->state.rendering = true;
	r->state.render_aborted = false;
	
	if (r->state.clients.count) logr(info, "Using %lu render worker%s totaling %lu thread%s.\n", r->state.clients.count, PLURAL(r->state.clients.count), r->state.clients.count, PLURAL(r->state.clients.count));
	
	// Select the appropriate renderer type for local use
	void *(*local_render_thread)(void *) = render_thread;
	// Iterative mode is incompatible with network rendering at the moment
	if (r->prefs.iterative && !r->state.clients.count) local_render_thread = render_thread_interactive;
	
	// Create & boot workers (Nonblocking)
	// Local render threads + one thread for every client
	for (size_t t = 0; t < r->prefs.threads; ++t) {
		worker_arr_add(&r->state.workers, (struct worker){
			.renderer = r,
			.buf = result,
			.cam = camera,
			.thread = (struct cr_thread){
				.thread_fn = local_render_thread,
			}
		});
	}
	for (size_t c = 0; c < r->state.clients.count; ++c) {
		worker_arr_add(&r->state.workers, (struct worker){
			.client = &r->state.clients.items[c],
			.renderer = r,
			.buf = result,
			.cam = camera,
			.thread = (struct cr_thread){
				.thread_fn = client_connection_thread
			}
		});
	}
	for (size_t w = 0; w < r->state.workers.count; ++w) {
		r->state.workers.items[w].thread.user_data = &r->state.workers.items[w];
		r->state.workers.items[w].tiles = &set;
		if (thread_start(&r->state.workers.items[w].thread))
			logr(error, "Failed to start worker %zu\n", w);
	}

	//Start main thread loop to handle renderer feedback and state management
	while (r->state.rendering) {
		if (g_aborted) {
			r->state.render_aborted = true;
		}
		
		struct callback status = r->state.callbacks[cr_cb_status_update];
		if (status.fn) {
			update_cb_info(r, &set, &cb_info);
			status.fn(&cb_info, status.user_data);
		}

		size_t inactive = 0;
		for (size_t w = 0; w < r->state.workers.count; ++w) {
			if (r->state.workers.items[w].thread_complete) inactive++;
		}
		if (r->state.render_aborted || inactive == r->state.workers.count)
			r->state.rendering = false;
		timer_sleep_ms(r->state.workers.items[0].paused ? paused_msec : active_msec);
	}

	r->state.current_set = NULL;
	
	//Make sure render threads are terminated before continuing (This blocks)
	for (size_t w = 0; w < r->state.workers.count; ++w) {
		thread_wait(&r->state.workers.items[w].thread);
	}
	struct callback stop = r->state.callbacks[cr_cb_on_stop];
	if (stop.fn) {
		update_cb_info(r, &set, &cb_info);
		if (r->state.render_aborted)
			cb_info.aborted = true;
		stop.fn(&cb_info, stop.user_data);
	}
	if (info_tiles) free(info_tiles);
	tile_set_free(&set);
	logr(info, "Renderer exiting\n");
	r->state.exit_done = true;
}

// An interactive render thread that progressively
// renders samples up to a limit
void *render_thread_interactive(void *arg) {
	block_signals();
	struct worker *threadState = arg;
	threadState->in_pause_loop = false;
	struct renderer *r = threadState->renderer;
	struct texture **buf = threadState->buf;
	sampler *sampler = newSampler();

	struct camera *cam = threadState->cam;
	
	//First time setup for each thread
	struct render_tile *tile = tile_next_interactive(r, threadState->tiles);
	threadState->currentTile = tile;
	
	struct timeval timer = {0};
	
	while (tile && r->state.rendering) {
		long total_us = 0;

		timer_start(&timer);
		for (int y = tile->end.y - 1; y > tile->begin.y - 1; --y) {
			for (int x = tile->begin.x; x < tile->end.x; ++x) {
				if (r->state.render_aborted) goto exit;
				uint32_t pixIdx = (uint32_t)(y * (*buf)->width + x);
				//FIXME: This does not converge to the same result as with regular renderThread.
				//I assume that's because we'd have to init the sampler differently when we render all
				//the tiles in one go per sample, instead of the other way around.
				initSampler(sampler, SAMPLING_STRATEGY, r->state.finishedPasses, r->prefs.sampleCount, pixIdx);
				
				struct color output = textureGetPixel(*buf, x, y, false);
				thread_rwlock_rdlock(&r->scene->bvh_lock);
				struct color sample = path_trace(cam_get_ray(cam, x, y, sampler), r->scene, r->prefs.bounces, sampler);
				thread_rwlock_unlock(&r->scene->bvh_lock);

				nan_clamp(&sample, &output);
				
				//And process the running average
				output = colorCoef((float)(r->state.finishedPasses - 1), output);
				output = colorAdd(output, sample);
				float t = 1.0f / r->state.finishedPasses;
				output = colorCoef(t, output);
				
				//Store internal render buffer (float precision)
				setPixel(*buf, output, x, y);
			}
		}
		//For performance metrics
		total_us += timer_get_us(timer);
		threadState->totalSamples++;
		threadState->avg_per_sample_us = total_us / r->state.finishedPasses;
		
		//Tile has finished rendering, get a new one and start rendering it.
		tile->state = finished;
		threadState->currentTile = NULL;
		tile = tile_next_interactive(r, threadState->tiles);
		//Pause rendering when bool is set
		while (threadState->paused && !r->state.render_aborted) {
			threadState->in_pause_loop = true;
			timer_sleep_ms(100);
		}
		threadState->in_pause_loop = false;
		// In case we got NULL back because we were paused:
		if (!tile) tile = tile_next_interactive(r, threadState->tiles);
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
void *render_thread(void *arg) {
	block_signals();
	struct worker *threadState = arg;
	struct renderer *r = threadState->renderer;
	struct texture **buf = threadState->buf;
	sampler *sampler = newSampler();

	struct camera *cam = threadState->cam;

	//First time setup for each thread
	struct render_tile *tile = tile_next(threadState->tiles);
	threadState->currentTile = tile;
	
	struct timeval timer = { 0 };
	size_t samples = 1;
	
	while (tile && r->state.rendering) {
		long total_us = 0;
		
		while (samples < r->prefs.sampleCount + 1 && r->state.rendering) {
			timer_start(&timer);
			for (int y = tile->end.y - 1; y > tile->begin.y - 1; --y) {
				for (int x = tile->begin.x; x < tile->end.x; ++x) {
					if (r->state.render_aborted) goto exit;
					uint32_t pixIdx = (uint32_t)(y * (*buf)->width + x);
					initSampler(sampler, SAMPLING_STRATEGY, samples - 1, r->prefs.sampleCount, pixIdx);
					
					struct color output = textureGetPixel(*buf, x, y, false);
					thread_rwlock_rdlock(&r->scene->bvh_lock);
					struct color sample = path_trace(cam_get_ray(cam, x, y, sampler), r->scene, r->prefs.bounces, sampler);
					thread_rwlock_unlock(&r->scene->bvh_lock);
					
					// Clamp out fireflies - This is probably not a good way to do that.
					nan_clamp(&sample, &output);

					//And process the running average
					output = colorCoef((float)(samples - 1), output);
					output = colorAdd(output, sample);
					float t = 1.0f / samples;
					output = colorCoef(t, output);
					
					//Store internal render buffer (float precision)
					setPixel(*buf, output, x, y);
				}
			}
			//For performance metrics
			total_us += timer_get_us(timer);
			threadState->totalSamples++;
			samples++;
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
		samples = 1;
		tile = tile_next(threadState->tiles);
		threadState->currentTile = tile;
	}
exit:
	destroySampler(sampler);
	//No more tiles to render, exit thread. (render done)
	threadState->thread_complete = true;
	threadState->currentTile = NULL;
	return 0;
}

struct prefs default_prefs(void) {
	return (struct prefs){
			.tileOrder = ro_from_middle,
			.threads = sys_get_cores() + 2,
			.sampleCount = 25,
			.bounces = 20,
			.tileWidth = 32,
			.tileHeight = 32,
			.imgFilePath = stringCopy("./"),
			.imgFileName = stringCopy("rendered"),
			.imgCount = 0,
	};
}

struct renderer *renderer_new(void) {
	struct renderer *r = calloc(1, sizeof(*r));
	r->prefs = default_prefs();
	r->state.finishedPasses = 1;
	
	// Move these elsewhere
	r->scene = calloc(1, sizeof(*r->scene));
	r->scene->asset_path = stringCopy("./");
	r->scene->storage.node_pool = newBlock(NULL, 1024);
	r->scene->storage.node_table = newHashtable(compareNodes, &r->scene->storage.node_pool);
	thread_rwlock_init(&r->scene->bvh_lock);
	r->scene->bvh_builder = thread_pool_create(sys_get_cores());
	return r;
}

void renderer_destroy(struct renderer *r) {
	if (!r) return;
	thread_pool_destroy(r->scene->bvh_builder);
	scene_destroy(r->scene);
	worker_arr_free(&r->state.workers);
	render_client_arr_free(&r->state.clients);
	free(r->prefs.imgFileName);
	free(r->prefs.imgFilePath);
	if (r->prefs.node_list) free(r->prefs.node_list);
	if (r->state.result_buf) destroyTexture(r->state.result_buf);
	free(r);
}
