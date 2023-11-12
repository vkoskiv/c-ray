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
#include "../utils/platform/sdl.h"
#include "samplers/sampler.h"
#include "../utils/args.h"
#include "../utils/platform/capabilities.h"
#include "../utils/platform/signal.h"
#include "../utils/protocol/server.h"
#include "../utils/string.h"

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


void *renderThread(void *arg);
void *renderThreadInteractive(void *arg);

/// @todo Use defaultSettings state struct for this.
/// @todo Clean this up, it's ugly.
struct texture *renderFrame(struct renderer *r) {
	//Check for CTRL-C
	if (registerHandler(sigint, sigHandler)) {
		logr(warning, "Unable to catch SIGINT\n");
	}
	struct camera camera = r->scene->cameras.items[r->prefs.selected_camera];
	struct texture *output = newTexture(char_p, camera.width, camera.height, 3);
	
	logr(info, "Starting c-ray renderer for frame %zu\n", r->prefs.imgCount);
	
	// Verify we have at least a single thread rendering.
	if (r->state.clientCount == 0 && r->prefs.threads < 1) {
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
	
	logr(info, "Pathtracing%s...\n", args_is_set("interactive") ? " iteratively" : "");
	
	r->state.rendering = true;
	r->state.render_aborted = false;
	r->state.saveImage = true; // Set to false if user presses X
	
	//Main loop (input)
	float avgSampleTime = 0.0f;
	float avgTimePerTilePass = 0.0f;
	int pauser = 0;
	int ctr = 1;
	bool interactive = args_is_set("interactive");
	
	size_t remoteThreads = 0;
	for (size_t i = 0; i < r->state.clientCount; ++i) {
		remoteThreads += r->state.clients[i].availableThreads;
	}
	
	if (r->state.clients) logr(info, "Using %lu render worker%s totaling %lu thread%s.\n", r->state.clientCount, PLURAL(r->state.clientCount), remoteThreads, PLURAL(remoteThreads));
	
	// Select the appropriate renderer type for local use
	void *(*localRenderThread)(void *) = renderThread;
	// Iterative mode is incompatible with network rendering at the moment
	if (interactive && !r->state.clients) localRenderThread = renderThreadInteractive;
	
	// Local render threads + one thread for every client
	size_t total_thread_count = r->prefs.threads + (int)r->state.clientCount;
	r->state.workers = calloc(total_thread_count, sizeof(*r->state.workers));

	//Allocate memory for render buffer
	//Render buffer is used to store accurate color values for the renderers' internal use
	r->state.renderBuffer = newTexture(float_p, camera.width, camera.height, 3);
	
	//Create & boot workers (Nonblocking)
	for (int t = 0; t < (int)total_thread_count; ++t) {
		r->state.workers[t] = (struct worker){
			.client = t > (int)r->prefs.threads - 1 ? &r->state.clients[t - r->prefs.threads] : NULL,
			.thread_complete = false,
			.renderer = r,
			.output = output,
			.cam = &camera,
			.thread = (struct cr_thread){
				.thread_fn = t > (int)r->prefs.threads - 1 ? networkRenderThread : localRenderThread,
				.user_data = &r->state.workers[t]
			}
		};
		if (thread_start(&r->state.workers[t].thread))
			logr(error, "Failed to start worker %d\n", t);
	}

	struct sdl_window *sdl = win_try_init(&r->prefs.window, camera.width, camera.height);
	
	//Start main thread loop to handle SDL and statistics computation
	//FIXME: Statistics computation is a gigantic mess. It will also break in the case
	//where a worker node disconnects during a render, so maybe fix that next.
	while (r->state.rendering) {
		win_check_keyboard(sdl, r);

		if (g_aborted) {
			r->state.saveImage = false;
			r->state.render_aborted = true;
		}
		
		//Gather and maintain this average constantly.
		if (!r->state.workers[0].paused) {
			win_update(sdl, r, output);
			for (size_t t = 0; t < total_thread_count; ++t) {
				avgSampleTime += r->state.workers[t].avgSampleTime;
			}
			avgTimePerTilePass += avgSampleTime / total_thread_count;
			avgTimePerTilePass /= ctr++;
		}
		
		//Run the sample printing about 4x/s
		if (pauser == 280 / active_msec) {
			float usPerRay = avgTimePerTilePass / (r->prefs.tileHeight * r->prefs.tileWidth);
			uint64_t completedSamples = 0;
			for (size_t t = 0; t < total_thread_count; ++t) {
				completedSamples += r->state.workers[t].totalSamples;
			}
			uint64_t remainingTileSamples = (r->state.tiles.count * r->prefs.sampleCount) - completedSamples;
			uint64_t msecTillFinished = 0.001f * (avgTimePerTilePass * remainingTileSamples);
			float sps = (1000000.0f / usPerRay) * (r->prefs.threads + remoteThreads);
			char rem[64];
			smartTime((msecTillFinished) / (r->prefs.threads + remoteThreads), rem);
			logr(info, "[%s%.0f%%%s] μs/path: %.02f, etf: %s, %.02lfMs/s %s        \r",
				 KBLU,
				 interactive ? ((double)r->state.finishedPasses / (double)r->prefs.sampleCount) * 100.0 :
							   ((double)r->state.finishedTileCount / (double)r->state.tiles.count) * 100.0,
				 KNRM,
				 (double)usPerRay,
				 rem,
				 0.000001 * (double)sps,
				 r->state.workers[0].paused ? "[PAUSED]" : "");
			
			pauser = 0;
		}
		pauser++;
		

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
	win_destroy(sdl);
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
		long totalUsec = 0;

		timer_start(&timer);
		for (int y = tile->end.y - 1; y > tile->begin.y - 1; --y) {
			for (int x = tile->begin.x; x < tile->end.x; ++x) {
				if (r->state.render_aborted) return 0;
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
		totalUsec += timer_get_us(timer);
		threadState->totalSamples++;
		threadState->completedSamples++;
		//Pause rendering when bool is set
		while (threadState->paused && !r->state.render_aborted) {
			timer_sleep_ms(100);
		}
		threadState->avgSampleTime = totalUsec / r->state.finishedPasses;
		
		//Tile has finished rendering, get a new one and start rendering it.
		tile->state = finished;
		threadState->currentTile = NULL;
		threadState->completedSamples = r->state.finishedPasses;
		tile = tile_next_interactive(r);
		threadState->currentTile = tile;
	}
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
		long totalUsec = 0;
		long samples = 0;
		
		while (threadState->completedSamples < r->prefs.sampleCount + 1 && r->state.rendering) {
			timer_start(&timer);
			for (int y = tile->end.y - 1; y > tile->begin.y - 1; --y) {
				for (int x = tile->begin.x; x < tile->end.x; ++x) {
					if (r->state.render_aborted) return 0;
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
			totalUsec += timer_get_us(timer);
			threadState->totalSamples++;
			threadState->completedSamples++;
			tile->completed_samples++;
			//Pause rendering when bool is set
			while (threadState->paused && !r->state.render_aborted) {
				timer_sleep_ms(100);
			}
			threadState->avgSampleTime = totalUsec / samples;
		}
		//Tile has finished rendering, get a new one and start rendering it.
		tile->state = finished;
		threadState->currentTile = NULL;
		threadState->completedSamples = 1;
		tile = tile_next(r);
		threadState->currentTile = tile;
	}
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
			.imgFileName = stringCopy("rendered"),
			.imgCount = 0,
			.override_dimensions = false,
			.imgType = png,
			.window = {
				.enabled = true,
				.fullscreen = false,
				.borderless = false,
				.scale = 1.0f
			}
	};
}

struct renderer *renderer_new() {
	struct renderer *r = calloc(1, sizeof(*r));
	r->prefs = defaults();
	r->state.finishedPasses = 1;
	
	r->state.tileMutex = mutex_create();
	if (args_is_set("use_clustering")) r->state.file_cache = calloc(1, sizeof(struct file_cache));

	r->scene = calloc(1, sizeof(*r->scene));
	r->scene->storage.node_pool = newBlock(NULL, 1024);
	r->scene->storage.node_table = newHashtable(compareNodes, &r->scene->storage.node_pool);
	return r;
}
	
void renderer_destroy(struct renderer *r) {
	if (r) {
		destroyScene(r->scene);
		if (r->state.renderBuffer) destroyTexture(r->state.renderBuffer);
		render_tile_arr_free(&r->state.tiles);
		free(r->state.workers);
		free(r->state.tileMutex);
		if (r->state.file_cache) {
			cache_destroy(r->state.file_cache);
			free(r->state.file_cache);
		}
		free(r->prefs.imgFileName);
		free(r->prefs.imgFilePath);
		free(r->prefs.assetPath);
		free(r);
	}
}
