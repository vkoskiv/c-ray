//
//  renderer.c
//  C-ray
//
//  Created by Valtteri Koskivuori on 19/02/2017.
//  Copyright © 2017-2020 Valtteri Koskivuori. All rights reserved.
//

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
#include "../datatypes/vertexbuffer.h"
#include "../utils/platform/thread.h"
#include "../utils/platform/mutex.h"
#include "samplers/sampler.h"
#include "../utils/args.h"
#include "../utils/platform/capabilities.h"
#include "../utils/protocol.h"

//Main thread loop speeds
#define paused_msec 100
#define active_msec  16

void *renderThread(void *arg);
void *renderThreadInteractive(void *arg);

/// @todo Use defaultSettings state struct for this.
/// @todo Clean this up, it's ugly.
struct texture *renderFrame(struct renderer *r) {
	struct texture *output = newTexture(char_p, r->prefs.imageWidth, r->prefs.imageHeight, 3);
	
	logr(info, "Starting C-ray renderer for frame %i\n", r->prefs.imgCount);
	
	bool threadsReduced = getSysCores() > r->prefs.threadCount;
	
	logr(info, "Rendering at %s%i%s x %s%i%s\n", KWHT, r->prefs.imageWidth, KNRM, KWHT, r->prefs.imageHeight, KNRM);
	logr(info, "Rendering %s%i%s samples with %s%i%s bounces.\n", KBLU, r->prefs.sampleCount, KNRM, KGRN, r->prefs.bounces, KNRM);
	logr(info, "Rendering with %s%d%s%s thread%s",
		 KRED,
		 r->prefs.fromSystem && !threadsReduced ? r->prefs.threadCount - 2 : r->prefs.threadCount,
		 r->prefs.fromSystem && !threadsReduced ? "+2" : "",
		 KNRM,
		 r->prefs.threadCount > 1 ? "s.\n" : ".\n");
	
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
	
	logr(info, "Using %lu render workers.\n", r->state.clientCount);
	
	// Local render threads + one thread for every client
	int totalThreadCount = r->prefs.threadCount + (int)r->state.clientCount;
	
	// Map of threads that have finished, so we don't check them again.
	bool *checkedThreads = calloc(totalThreadCount, sizeof(*checkedThreads));
	
	r->state.threads = calloc(totalThreadCount, sizeof(*r->state.threads));
	r->state.threadStates = calloc(totalThreadCount, sizeof(*r->state.threadStates));
	
	//Create render threads (Nonblocking)
	for (int t = 0; t < r->prefs.threadCount; ++t) {
		r->state.threadStates[t] = (struct renderThreadState){.thread_num = t, .threadComplete = false, .renderer = r, .output = output};
		r->state.threads[t] = (struct crThread){.threadFunc = renderThread, .userData = &r->state.threadStates[t]};
		if (threadStart(&r->state.threads[t])) {
			logr(error, "Failed to create a render thread.\n");
		} else {
			r->state.activeThreads++;
		}
	}
	
	// Create network worker manager threads
	for (int t = 0; t < (int)r->state.clientCount; ++t) {
		int offset = r->prefs.threadCount + t;
		r->state.threadStates[offset] = (struct renderThreadState){.client = r->state.clients, .thread_num = offset, .threadComplete = false, .renderer = r, .output = output};
		r->state.threads[offset] = (struct crThread){.threadFunc = networkRenderThread, .userData = &r->state.threadStates[offset]};
		if (threadStart(&r->state.threads[offset])) {
			logr(error, "Failed to create a network thread.\n");
		} else {
			r->state.activeThreads++;
		}
	}
	
	//Start main thread loop to handle SDL and statistics computation
	while (r->state.isRendering) {
		getKeyboardInput(r);
		
		//Gather and maintain this average constantly.
		if (!r->state.threadStates[0].paused) {
			drawWindow(r, output);
			for (int t = 0; t < r->prefs.threadCount; ++t) {
				avgSampleTime += r->state.threadStates[t].avgSampleTime;
			}
			avgTimePerTilePass += avgSampleTime / r->prefs.threadCount;
			avgTimePerTilePass /= ctr++;
		}
		
		//Run the sample printing about 4x/s
		if (pauser == 280 / active_msec) {
			float usPerRay = avgTimePerTilePass / (r->prefs.tileHeight * r->prefs.tileWidth);
			uint64_t completedSamples = 0;
			for (int t = 0; t < r->prefs.threadCount; ++t) {
				completedSamples += r->state.threadStates[t].totalSamples;
			}
			uint64_t remainingTileSamples = (r->state.tileCount * r->prefs.sampleCount) - completedSamples;
			uint64_t msecTillFinished = 0.001f * (avgTimePerTilePass * remainingTileSamples);
			float sps = (1000000.0f/usPerRay) * r->prefs.threadCount;
			char rem[64];
			smartTime((msecTillFinished) / r->prefs.threadCount, rem);
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
		for (int t = 0; t < totalThreadCount; ++t) {
			if (r->state.threadStates[t].threadComplete && !checkedThreads[t]) {
				--r->state.activeThreads;
				checkedThreads[t] = true; //Mark as checked
			}
			if (!r->state.activeThreads || r->state.renderAborted) {
				r->state.isRendering = false;
			}
		}
		sleepMSec(r->state.threadStates[0].paused ? paused_msec : active_msec);
	}
	
	//Make sure render threads are terminated before continuing (This blocks)
	for (int t = 0; t < r->prefs.threadCount; ++t) {
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
	
	//First time setup for each thread
	struct renderTile tile = nextTile(r);
	threadState->currentTileNum = tile.tileNum;
	
	struct timeval timer = {0};
	
	threadState->completedSamples = 1;
	
	while (r->state.finishedPasses < r->prefs.sampleCount && r->state.isRendering) {
		long totalUsec = 0;
		
		startTimer(&timer);
		for (int y = tile.end.y - 1; y > tile.begin.y - 1; --y) {
			for (int x = tile.begin.x; x < tile.end.x; ++x) {
				if (r->state.renderAborted) return 0;
				uint32_t pixIdx = (uint32_t)(y * image->width + x);
				initSampler(sampler, Halton, r->state.finishedPasses, r->prefs.sampleCount, pixIdx);
				
				struct color output = textureGetPixel(r->state.renderBuffer, x, y, false);
				struct lightRay incidentRay = getCameraRay(r->scene->camera, x, y, sampler);
				struct color sample = pathTrace(&incidentRay, r->scene, r->prefs.bounces, sampler);
				
				//And process the running average
				output = colorCoef((float)(r->state.finishedPasses - 1), output);
				output = addColors(output, sample);
				float t = 1.0f / r->state.finishedPasses;
				output = colorCoef(t, output);
				
				//Store internal render buffer (float precision)
				setPixel(r->state.renderBuffer, output, x, y);
				
				//Gamma correction
				output = toSRGB(output);
				
				//And store the image data
				setPixel(image, output, x, y);
			}
		}
		//For performance metrics
		totalUsec += getUs(timer);
		threadState->totalSamples++;
		threadState->completedSamples++;
		//Pause rendering when bool is set
		while (threadState->paused && !r->state.renderAborted) {
			sleepMSec(100);
		}
		threadState->avgSampleTime = totalUsec / r->state.finishedPasses;
		
		//Tile has finished rendering, get a new one and start rendering it.
		if (tile.tileNum != -1) r->state.renderTiles[tile.tileNum].isRendering = false;
		threadState->currentTileNum = -1;
		threadState->completedSamples = r->state.finishedPasses;
		tile = nextTileInteractive(r);
		threadState->currentTileNum = tile.tileNum;
	}
	destroySampler(sampler);
	//No more tiles to render, exit thread. (render done)
	threadState->threadComplete = true;
	threadState->currentTileNum = -1;
	return 0;
}

struct texture *renderSingleTile(struct renderer *r, struct renderTile tile) {
	struct texture *tileData = newTexture(char_p, tile.width, tile.height, 3);
	sampler *sampler = newSampler();
	int completedSamples = 1;
	long samples = 0;
	while (completedSamples < r->prefs.sampleCount+1 && r->state.isRendering) {
		for (int y = tile.end.y - 1; y > tile.begin.y - 1; --y) {
			for (int x = tile.begin.x; x < tile.end.x; ++x) {
				if (r->state.renderAborted) return 0;
				uint32_t pixIdx = (uint32_t)(y * r->prefs.imageWidth + x);
				initSampler(sampler, Halton, completedSamples - 1, r->prefs.sampleCount, pixIdx);
				
				struct color output = textureGetPixel(r->state.renderBuffer, x, y, false);
				struct lightRay incidentRay = getCameraRay(r->scene->camera, x, y, sampler);
				struct color sample = pathTrace(&incidentRay, r->scene, r->prefs.bounces, sampler);
				
				//And process the running average
				output = colorCoef((float)(completedSamples - 1), output);
				output = addColors(output, sample);
				float t = 1.0f / completedSamples;
				output = colorCoef(t, output);
				
				//Store internal render buffer (float precision)
				setPixel(r->state.renderBuffer, output, x, y);
				
				//Gamma correction
				output = toSRGB(output);
				
				//And store the image data
				int localX = x - tile.begin.x;
				int localY = y - tile.begin.y;
				setPixel(tileData, output, localX, localY);
			}
		}
		//For performance metrics
		samples++;
		completedSamples++;
	}
	return tileData;
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
	
	//First time setup for each thread
	struct renderTile tile = nextTile(r);
	threadState->currentTileNum = tile.tileNum;
	
	struct timeval timer = {0};
	threadState->completedSamples = 1;
	
	while (tile.tileNum != -1 && r->state.isRendering) {
		long totalUsec = 0;
		long samples = 0;
		
		while (threadState->completedSamples < r->prefs.sampleCount+1 && r->state.isRendering) {
			startTimer(&timer);
			for (int y = tile.end.y - 1; y > tile.begin.y - 1; --y) {
				for (int x = tile.begin.x; x < tile.end.x; ++x) {
					if (r->state.renderAborted) return 0;
					uint32_t pixIdx = (uint32_t)(y * image->width + x);
					initSampler(sampler, Halton, threadState->completedSamples - 1, r->prefs.sampleCount, pixIdx);
					
					struct color output = textureGetPixel(r->state.renderBuffer, x, y, false);
					struct lightRay incidentRay = getCameraRay(r->scene->camera, x, y, sampler);
					struct color sample = pathTrace(&incidentRay, r->scene, r->prefs.bounces, sampler);
					
					//And process the running average
					output = colorCoef((float)(threadState->completedSamples - 1), output);
					output = addColors(output, sample);
					float t = 1.0f / threadState->completedSamples;
					output = colorCoef(t, output);
					
					//Store internal render buffer (float precision)
					setPixel(r->state.renderBuffer, output, x, y);
					
					//Gamma correction
					output = toSRGB(output);
					
					//And store the image data
					setPixel(image, output, x, y);
				}
			}
			//For performance metrics
			samples++;
			totalUsec += getUs(timer);
			threadState->totalSamples++;
			threadState->completedSamples++;
			//Pause rendering when bool is set
			while (threadState->paused && !r->state.renderAborted) {
				sleepMSec(100);
			}
			threadState->avgSampleTime = totalUsec / samples;
		}
		//Tile has finished rendering, get a new one and start rendering it.
		r->state.renderTiles[tile.tileNum].isRendering = false;
		r->state.renderTiles[tile.tileNum].renderComplete = true;
		threadState->currentTileNum = -1;
		threadState->completedSamples = 1;
		tile = nextTile(r);
		threadState->currentTileNum = tile.tileNum;
	}
	destroySampler(sampler);
	//No more tiles to render, exit thread. (render done)
	threadState->threadComplete = true;
	threadState->currentTileNum = -1;
	return 0;
}

struct renderer *newRenderer() {
	struct renderer *r = calloc(1, sizeof(*r));
	r->state.avgTileTime = (time_t)1;
	r->state.timeSampleCount = 1;
	r->state.finishedPasses = 1;
	
	r->state.timer = calloc(1, sizeof(*r->state.timer));
	
	if (!g_vertices) {
		allocVertexBuffers();
	}
	
	//Mutex
	r->state.tileMutex = createMutex();
	return r;
}
	
void destroyRenderer(struct renderer *r) {
	if (r) {
		destroyScene(r->scene);
		destroyTexture(r->state.renderBuffer);
		destroyTexture(r->state.uiBuffer);
		destroyVertexBuffers();
		free(r->state.timer);
		free(r->state.renderTiles);
		free(r->state.threads);
		free(r->state.threadStates);
		free(r->state.tileMutex);
		free(r->prefs.imgFileName);
		free(r->prefs.imgFilePath);
		free(r->prefs.assetPath);
		free(r);
	}
}
