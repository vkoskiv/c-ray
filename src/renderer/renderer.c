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
#include "../utils/ui.h"
#include "../datatypes/tile.h"
#include "../utils/timer.h"
#include "../datatypes/image/texture.h"
#include "../datatypes/mesh.h"
#include "../datatypes/sphere.h"
#include "../utils/platform/thread.h"
#include "../utils/platform/mutex.h"
#include "samplers/sampler.h"
#include "../utils/args.h"
#include "../utils/platform/capabilities.h"
#include "../utils/protocol/server.h"
#include "../utils/string.h"

//Main thread loop speeds
#define paused_msec 100
#define active_msec  16

void *renderThread(void *arg);
void *renderThreadInteractive(void *arg);

/// @todo Use defaultSettings state struct for this.
/// @todo Clean this up, it's ugly.
struct texture *renderFrame(struct renderer *r) {
	struct camera camera = r->scene->cameras[r->prefs.selected_camera];
	struct texture *output = newTexture(char_p, camera.width, camera.height, 3);
	
	logr(info, "Starting C-ray renderer for frame %i\n", r->prefs.imgCount);
	
	// Verify we have at least a single thread rendering.
	if (r->state.clientCount == 0 && r->prefs.threadCount < 1) {
		logr(warning, "No network render workers, setting thread count to 1\n");
		r->prefs.threadCount = 1;
	}
	
	bool threadsReduced = getSysCores() > r->prefs.threadCount;
	
	logr(info, "Rendering at %s%i%s x %s%i%s\n", KWHT, camera.width, KNRM, KWHT, camera.height, KNRM);
	logr(info, "Rendering %s%i%s samples with %s%i%s bounces.\n", KBLU, r->prefs.sampleCount, KNRM, KGRN, r->prefs.bounces, KNRM);
	logr(info, "Rendering with %s%d%s%s thread%s.\n",
		 KRED,
		 r->prefs.fromSystem && !threadsReduced ? r->prefs.threadCount - 2 : r->prefs.threadCount,
		 r->prefs.fromSystem && !threadsReduced ? "+2" : "",
		 KNRM,
		 PLURAL(r->prefs.threadCount));
	
	logr(info, "Pathtracing%s...\n", isSet("interactive") ? " iteratively" : "");
	
	r->state.isRendering = true;
	r->state.renderAborted = false;
	r->state.saveImage = true; // Set to false if user presses X
	
	//Main loop (input)
	float avgSampleTime = 0.0f;
	float avgTimePerTilePass = 0.0f;
	int pauser = 0;
	int ctr = 1;
	bool interactive = isSet("interactive");
	
	size_t remoteThreads = 0;
	for (size_t i = 0; i < r->state.clientCount; ++i) {
		remoteThreads += r->state.clients[i].availableThreads;
	}
	
	if (r->state.clients) logr(info, "Using %lu render worker%s totaling %lu thread%s.\n", r->state.clientCount, PLURAL(r->state.clientCount), remoteThreads, PLURAL(remoteThreads));
	
	// Local render threads + one thread for every client
	int localThreadCount = r->prefs.threadCount + (int)r->state.clientCount;
	
	// Map of threads that have finished, so we don't check them again.
	bool *checkedThreads = calloc(localThreadCount, sizeof(*checkedThreads));
	
	r->state.threads = calloc(localThreadCount, sizeof(*r->state.threads));
	r->state.threadStates = calloc(localThreadCount, sizeof(*r->state.threadStates));
	
	// Select the appropriate renderer type for local use
	void *(*localRenderThread)(void *) = renderThread;
	// Iterative mode is incompatible with network rendering at the moment
	if (interactive && !r->state.clients) localRenderThread = renderThreadInteractive;
	
	//Create render threads (Nonblocking)
	for (int t = 0; t < r->prefs.threadCount; ++t) {
		r->state.threadStates[t] = (struct renderThreadState){.thread_num = t, .threadComplete = false, .renderer = r, .output = output, .cam = &camera};
		r->state.threads[t] = (struct crThread){.threadFunc = localRenderThread, .userData = &r->state.threadStates[t]};
		if (threadStart(&r->state.threads[t])) {
			logr(error, "Failed to create a render thread.\n");
		} else {
			r->state.activeThreads++;
		}
	}
	
	// Create network worker manager threads
	for (int t = 0; t < (int)r->state.clientCount; ++t) {
		int offset = r->prefs.threadCount + t;
		r->state.threadStates[offset] = (struct renderThreadState){.client = &r->state.clients[t], .thread_num = offset, .threadComplete = false, .renderer = r, .output = output};
		r->state.threads[offset] = (struct crThread){.threadFunc = networkRenderThread, .userData = &r->state.threadStates[offset]};
		if (threadStart(&r->state.threads[offset])) {
			logr(error, "Failed to create a network thread.\n");
		} else {
			r->state.activeThreads++;
		}
	}
	
	//Start main thread loop to handle SDL and statistics computation
	//FIXME: Statistics computation is a gigantic mess. It will also break in the case
	//where a worker node disconnects during a render, so maybe fix that next.
	while (r->state.isRendering) {
		getKeyboardInput(r);
		
		//Gather and maintain this average constantly.
		if (!r->state.threadStates[0].paused) {
			drawWindow(r, output);
			for (int t = 0; t < localThreadCount; ++t) {
				avgSampleTime += r->state.threadStates[t].avgSampleTime;
			}
			avgTimePerTilePass += avgSampleTime / localThreadCount;
			avgTimePerTilePass /= ctr++;
		}
		
		//Run the sample printing about 4x/s
		if (pauser == 280 / active_msec) {
			float usPerRay = avgTimePerTilePass / (r->prefs.tileHeight * r->prefs.tileWidth);
			uint64_t completedSamples = 0;
			for (int t = 0; t < localThreadCount; ++t) {
				completedSamples += r->state.threadStates[t].totalSamples;
			}
			uint64_t remainingTileSamples = (r->state.tileCount * r->prefs.sampleCount) - completedSamples;
			uint64_t msecTillFinished = 0.001f * (avgTimePerTilePass * remainingTileSamples);
			float sps = (1000000.0f / usPerRay) * (r->prefs.threadCount + remoteThreads);
			char rem[64];
			smartTime((msecTillFinished) / (r->prefs.threadCount + remoteThreads), rem);
			logr(info, "[%s%.0f%%%s] μs/path: %.02f, etf: %s, %.02lfMs/s %s        \r",
				 KBLU,
				 interactive ? ((float)r->state.finishedPasses / (float)r->prefs.sampleCount) * 100.0f :
							   ((float)r->state.finishedTileCount / (float)r->state.tileCount) * 100.0f,
				 KNRM,
				 usPerRay,
				 rem,
				 0.000001f * sps,
				 r->state.threadStates[0].paused ? "[PAUSED]" : "");
			
			pauser = 0;
		}
		pauser++;
		
		//Wait for render threads to finish (Render finished)
		for (int t = 0; t < localThreadCount; ++t) {
			if (r->state.threadStates[t].threadComplete && !checkedThreads[t]) {
				--r->state.activeThreads;
				checkedThreads[t] = true; //Mark as checked
			}
			if (!r->state.activeThreads || r->state.renderAborted) {
				r->state.isRendering = false;
			}
		}
		timer_sleep_ms(r->state.threadStates[0].paused ? paused_msec : active_msec);
	}
	
	//Make sure render threads are terminated before continuing (This blocks)
	for (int t = 0; t < localThreadCount; ++t) {
		threadWait(&r->state.threads[t]);
	}
	free(checkedThreads);
	return output;
}

// An interactive render thread that progressively
// renders samples up to a limit
void *renderThreadInteractive(void *arg) {
	struct renderThreadState *threadState = (struct renderThreadState*)threadUserData(arg);
	struct renderer *r = threadState->renderer;
	struct texture *image = threadState->output;
	sampler *sampler = newSampler();

	struct camera *cam = threadState->cam;
	
	//First time setup for each thread
	struct renderTile *tile = nextTileInteractive(r);
	threadState->currentTile = tile;
	
	struct timeval timer = {0};
	
	threadState->completedSamples = 1;
	
	while (tile && r->state.isRendering) {
		long totalUsec = 0;

		timer_start(&timer);
		for (int y = tile->end.y - 1; y > tile->begin.y - 1; --y) {
			for (int x = tile->begin.x; x < tile->end.x; ++x) {
				if (r->state.renderAborted) return 0;
				uint32_t pixIdx = (uint32_t)(y * image->width + x);
				//FIXME: This does not converge to the same result as with regular renderThread.
				//I assume that's because we'd have to init the sampler differently when we render all
				//the tiles in one go per sample, instead of the other way around.
				initSampler(sampler, SAMPLING_STRATEGY, r->state.finishedPasses, r->prefs.sampleCount, pixIdx);
				
				struct color output = textureGetPixel(r->state.renderBuffer, x, y, false);
				struct lightRay incidentRay = cam_get_ray(cam, x, y, sampler);
				struct color sample = pathTrace(&incidentRay, r->scene, r->prefs.bounces, sampler);
				
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
		while (threadState->paused && !r->state.renderAborted) {
			timer_sleep_ms(100);
		}
		threadState->avgSampleTime = totalUsec / r->state.finishedPasses;
		
		//Tile has finished rendering, get a new one and start rendering it.
		tile->state = finished;
		threadState->currentTile = NULL;
		threadState->completedSamples = r->state.finishedPasses;
		tile = nextTileInteractive(r);
		threadState->currentTile = tile;
	}
	destroySampler(sampler);
	//No more tiles to render, exit thread. (render done)
	threadState->threadComplete = true;
	threadState->currentTile = NULL;
	return 0;
}

/**
 A render thread
 
 @param arg Thread information (see threadInfo struct)
 @return Exits when thread is done
 */
void *renderThread(void *arg) {
	struct renderThreadState *threadState = (struct renderThreadState*)threadUserData(arg);
	struct renderer *r = threadState->renderer;
	struct texture *image = threadState->output;
	sampler *sampler = newSampler();

	struct camera *cam = threadState->cam;

	//First time setup for each thread
	struct renderTile *tile = nextTile(r);
	threadState->currentTile = tile;
	
	struct timeval timer = {0};
	threadState->completedSamples = 1;
	
	while (tile && r->state.isRendering) {
		long totalUsec = 0;
		long samples = 0;
		
		while (threadState->completedSamples < r->prefs.sampleCount + 1 && r->state.isRendering) {
			timer_start(&timer);
			for (int y = tile->end.y - 1; y > tile->begin.y - 1; --y) {
				for (int x = tile->begin.x; x < tile->end.x; ++x) {
					if (r->state.renderAborted) return 0;
					uint32_t pixIdx = (uint32_t)(y * image->width + x);
					initSampler(sampler, SAMPLING_STRATEGY, threadState->completedSamples - 1, r->prefs.sampleCount, pixIdx);
					
					struct color output = textureGetPixel(r->state.renderBuffer, x, y, false);
					struct lightRay incidentRay = cam_get_ray(cam, x, y, sampler);
					struct color sample = pathTrace(&incidentRay, r->scene, r->prefs.bounces, sampler);
					
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
			//Pause rendering when bool is set
			while (threadState->paused && !r->state.renderAborted) {
				timer_sleep_ms(100);
			}
			threadState->avgSampleTime = totalUsec / samples;
		}
		//Tile has finished rendering, get a new one and start rendering it.
		tile->state = finished;
		threadState->currentTile = NULL;
		threadState->completedSamples = 1;
		tile = nextTile(r);
		threadState->currentTile = tile;
	}
	destroySampler(sampler);
	//No more tiles to render, exit thread. (render done)
	threadState->threadComplete = true;
	threadState->currentTile = NULL;
	return 0;
}

static struct prefs defaultPrefs() {
	return (struct prefs){
			.tileOrder = renderOrderFromMiddle,
			.threadCount = getSysCores() + 2,
			.fromSystem = true,
			.sampleCount = 25,
			.bounces = 20,
			.tileWidth = 32,
			.tileHeight = 32,
			.imgFilePath = stringCopy("./"),
			.imgFileName = stringCopy("rendered"),
			.imgCount = 0,
			.override_dimensions = false,
			.override_width = 1280,
			.override_height = 800,
			.imgType = png,
			.enabled = true,
			.fullscreen = false,
			.borderless = false,
			.scale = 1.0f
	};
}

struct renderer *newRenderer() {
	struct renderer *r = calloc(1, sizeof(*r));
	r->prefs = defaultPrefs();
	r->state.avgTileTime = (time_t)1;
	r->state.timeSampleCount = 1;
	r->state.finishedPasses = 1;
	
	r->state.tileMutex = mutex_create();
	if (isSet("use_clustering")) r->state.file_cache = calloc(1, sizeof(struct file_cache));

	r->scene = calloc(1, sizeof(*r->scene));
	r->scene->storage.node_pool = newBlock(NULL, 1024);
	r->scene->storage.node_table = newHashtable(compareNodes, &r->scene->storage.node_pool);
	return r;
}
	
void destroyRenderer(struct renderer *r) {
	if (r) {
		destroyScene(r->scene);
		destroyTexture(r->state.renderBuffer);
		destroyTexture(r->state.uiBuffer);
		free(r->state.renderTiles);
		free(r->state.threads);
		free(r->state.threadStates);
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
