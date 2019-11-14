//
//  renderer.c
//  C-ray
//
//  Created by Valtteri Koskivuori on 19/02/2017.
//  Copyright © 2015-2019 Valtteri Koskivuori. All rights reserved.
//

#include "../includes.h"
#include "renderer.h"

#include "../datatypes/camera.h"
#include "../datatypes/scene.h"
#include "pathtrace.h"
#include "../utils/logging.h"
#include "../utils/ui.h"
#include "../datatypes/tile.h"
#include "../utils/timer.h"
#include "../datatypes/texture.h"
#include "../utils/loaders/textureloader.h"

#define paused_msec 800
#define active_msec  40

/// @todo Use defaultSettings state struct for this.
/// @todo Clean this up, it's ugly.
void render(struct renderer *r) {
	logr(info, "Starting C-ray renderer for frame %i\n", r->state.image->count);
	
	logr(info, "Rendering at %i x %i\n", r->state.image->width, r->state.image->height);
	logr(info, "Rendering %i samples with %i bounces.\n", r->prefs.sampleCount, r->prefs.bounces);
	logr(info, "Rendering with %d thread", r->prefs.threadCount);
	printf(r->prefs.threadCount > 1 ? "s.\n" : ".\n");
	
	logr(info, "Pathtracing\n");
	
	//Create threads
	int t;
	
	r->state.isRendering = true;
	r->state.renderAborted = false;
#ifndef WINDOWS
	pthread_attr_init(&r->state.renderThreadAttributes);
	pthread_attr_setdetachstate(&r->state.renderThreadAttributes, PTHREAD_CREATE_JOINABLE);
#endif
	//Main loop (input)
	bool threadsHaveStarted = false;
	float avgRayTime = 0.0f;
	int pauser = 0;
	float finalAvg = 0.0f;
	int ctr = 1;
	while (r->state.isRendering) {
		getKeyboardInput(r);
		drawWindow(r);
		
		for (int t = 0; t < r->prefs.threadCount; t++) {
			avgRayTime += r->state.threadStates[t].avgRayTime;
		}
		finalAvg += avgRayTime / r->prefs.threadCount;
		finalAvg /= ctr;
		ctr++;
		
		//Run the sample printing about 4x/s
		if (pauser == 280 / active_msec) {
			long remainingSampleCount = ((r->state.tileCount - r->state.finishedTileCount) * r->prefs.tileWidth * r->prefs.tileHeight * r->prefs.sampleCount);
			double sps = (1000000.0f/finalAvg) * r->prefs.threadCount;
			long usecTillFinished = remainingSampleCount * finalAvg;
			char rem[32];
			smartTime((0.001 * usecTillFinished) / r->prefs.threadCount, rem);
			float completion = ((float)r->state.finishedTileCount / r->state.tileCount) * 100;
			logr(info, "[%s%.0f%%%s] μs/ray: %.02f, etf: %s, %.02lfMs/s        \r", KBLU, KNRM, completion, finalAvg, rem, 0.000001*sps);
			pauser = 0;
		}
		pauser++;
		
		if (!threadsHaveStarted) {
			threadsHaveStarted = true;
			//Create render threads
			for (t = 0; t < r->prefs.threadCount; t++) {
				r->state.threadStates[t].thread_num = t;
				r->state.threadStates[t].threadComplete = false;
				r->state.threadStates[t].r = r;
				r->state.activeThreads++;
#ifdef WINDOWS
				DWORD threadId;
				r->state.threadStates[t].thread_handle = CreateThread(NULL, 0, renderThread, &r->state.threadStates[t], 0, &threadId);
				if (r->state.threadStates[t].thread_handle == NULL) {
					logr(error, "Failed to create thread.\n");
					exit(-1);
				}
				r->state.threadStates[t].thread_id = threadId;
#else
				if (pthread_create(&r->state.threadStates[t].thread_id, &r->state.renderThreadAttributes, renderThread, &r->state.threadStates[t])) {
					logr(error, "Failed to create a thread.\n");
				}
#endif
			}
			
			r->state.threadStates->threadComplete = false;
			
#ifndef WINDOWS
			if (pthread_attr_destroy(&r->state.renderThreadAttributes)) {
				logr(warning, "Failed to destroy pthread.\n");
			}
#endif
		}
		
		//Wait for render threads to finish (Render finished)
		for (t = 0; t < r->prefs.threadCount; t++) {
			if (r->state.threadStates[t].threadComplete && r->state.threadStates[t].thread_num != -1) {
				r->state.activeThreads--;
				r->state.threadStates[t].thread_num = -1;
			}
			if (r->state.activeThreads == 0 || r->state.renderAborted) {
				r->state.isRendering = false;
			}
		}
		if (r->state.threadPaused[0]) {
			sleepMSec(paused_msec);
		} else {
			sleepMSec(active_msec);
		}
	}
	
	//Make sure render threads are finished before continuing
	for (t = 0; t < r->prefs.threadCount; t++) {
#ifdef WINDOWS
		WaitForSingleObjectEx(r->state.threadStates[t].thread_handle, INFINITE, FALSE);
#else
		if (pthread_join(r->state.threadStates[t].thread_id, NULL)) {
			logr(warning, "Thread %t frozen.", t);
		}
#endif
	}
}

uint64_t hash(uint64_t x) {
	x = (x ^ (x >> 30)) * UINT64_C(0xbf58476d1ce4e5b9);
	x = (x ^ (x >> 27)) * UINT64_C(0x94d049bb133111eb);
	x = x ^ (x >> 31);
	return x;
}

/**
 A render thread
 
 @param arg Thread information (see threadInfo struct)
 @return Exits when thread is done
 */
#ifdef WINDOWS
DWORD WINAPI renderThread(LPVOID arg) {
#else
void *renderThread(void *arg) {
#endif
	struct lightRay incidentRay;
	struct threadState *tinfo = (struct threadState*)arg;
	
	struct renderer *r = tinfo->r;
	pcg32_random_t rng;
	
	//First time setup for each thread
	struct renderTile tile = getTile(r);
	tinfo->currentTileNum = tile.tileNum;
	
	bool hasHitObject = false;
	struct timeval timer = {0};
	
	while (tile.tileNum != -1 && r->state.isRendering) {
		unsigned long long sleepMs = 0;
		hasHitObject = false;
		long totalUsec = 0;
		long rays = 0;
		
		while (tile.completedSamples < r->prefs.sampleCount+1 && r->state.isRendering) {
			for (int y = (int)tile.end.y; y > (int)tile.begin.y; y--) {
				for (int x = (int)tile.begin.x; x < (int)tile.end.x; x++) {
					startTimer(&timer);
					uint64_t pixIdx = y * r->state.image->width + x;
					uint64_t idx = pixIdx * r->prefs.sampleCount + tile.completedSamples;
					pcg32_srandom_r(&rng, hash(idx), 0);
					
					float fracX = (float)x;
					float fracY = (float)y;
					
					//A cheap 'antialiasing' of sorts. The more samples, the better this works
					float jitter = 0.25;
					if (r->prefs.antialiasing) {
						fracX = rndFloatRange(fracX - jitter, fracX + jitter, &rng);
						fracY = rndFloatRange(fracY - jitter, fracY + jitter, &rng);
					}
					
					//Set up the light ray to be casted. direction is pointing towards the X,Y coordinate on the
					//imaginary plane in front of the origin. startPos is just the camera position.
					struct vector direction = {(fracX - 0.5 * r->state.image->width)
												/ r->scene->camera->focalLength,
											   (fracY - 0.5 * r->state.image->height)
												/ r->scene->camera->focalLength,
												1.0};
					
					//Normalize direction
					direction = vecNormalize(direction);
					struct vector startPos = r->scene->camera->pos;
					struct vector left = r->scene->camera->left;
					struct vector up = r->scene->camera->up;
					
					//Run camera tranforms on direction vector
					transformCameraView(r->scene->camera, &direction);
					
					//Now handle aperture
					//FIXME: This is a 'square' aperture
					float aperture = r->scene->camera->aperture;
					if (aperture <= 0.0) {
						incidentRay.start = startPos;
					} else {
						float randY = rndFloatRange(-aperture, aperture, &rng);
						float randX = rndFloatRange(-aperture, aperture, &rng);
						struct vector randomStart = vecAdd(vecAdd(startPos, vecMultiplyConst(up, randY)), vecMultiplyConst(left, randX));
						
						incidentRay.start = randomStart;
					}
					
					incidentRay.direction = direction;
					incidentRay.rayType = rayTypeIncident;
					incidentRay.remainingInteractions = r->prefs.bounces;
					incidentRay.currentMedium.IOR = AIR_IOR;
					
					//For multi-sample rendering, we keep a running average of color values for each pixel
					//The next block of code does this
					
					//Get previous color value from render buffer
					struct color output = textureGetPixel(r->state.renderBuffer, x, y);
					
					//Get new sample (path tracing is initiated here)
					struct color sample = pathTrace(&incidentRay, r->scene, 0, r->prefs.bounces, &rng, &hasHitObject);
					
					//And process the running average
					output.red = output.red * (tile.completedSamples - 1);
					output.green = output.green * (tile.completedSamples - 1);
					output.blue = output.blue * (tile.completedSamples - 1);
					
					output = addColors(output, sample);
					
					output.red = output.red / tile.completedSamples;
					output.green = output.green / tile.completedSamples;
					output.blue = output.blue / tile.completedSamples;
					
					//Store internal render buffer (float precision)
					blit(r->state.renderBuffer, output, x, y);
					
					//Gamma correction
					output = toSRGB(output);
					
					//And store the image data
					blit(r->state.image, output, x, y);
					
					//For performance metrics
					rays++;
					totalUsec += getUs(timer);
				}
			}
			tile.completedSamples++;
			tinfo->completedSamples = tile.completedSamples;
			if (tile.completedSamples > 25 && !hasHitObject) break; //Abort if we didn't hit anything within 25 samples
			//Pause rendering when bool is set
			while (r->state.threadPaused[tinfo->thread_num] && !r->state.renderAborted) {
				sleepMSec(100);
				sleepMs += 100;
			}
			tinfo->avgRayTime = totalUsec / rays;
		}
		//Tile has finished rendering, get a new one and start rendering it.
		r->state.renderTiles[tile.tileNum].isRendering = false;
		r->state.renderTiles[tile.tileNum].renderComplete = true;
		tinfo->currentTileNum = -1;
		tinfo->completedSamples = 0;
		tile = getTile(r);
		tinfo->currentTileNum = tile.tileNum;
	}
	//No more tiles to render, exit thread. (render done)
	tinfo->threadComplete = true;
	tinfo->currentTileNum = -1;
#ifdef WINDOWS
	return 0;
#else
	pthread_exit((void*) arg);
#endif
}
	
struct renderer *newRenderer() {
	struct renderer *r = calloc(1, sizeof(struct renderer));
	r->state.avgTileTime = (time_t)1;
	r->state.timeSampleCount = 1;
	r->prefs.fileMode = saveModeNormal;
	r->state.image = newTexture();
	
	r->scene = calloc(1, sizeof(struct world));
	r->scene->camera = calloc(1, sizeof(struct camera));
	r->scene->ambientColor = calloc(1, sizeof(struct gradient));
	r->scene->hdr = NULL; //Optional, to be loaded later
	r->scene->meshes = calloc(1, sizeof(struct mesh));
	r->scene->spheres = calloc(1, sizeof(struct sphere));
	
#ifdef UI_ENABLED
	r->mainDisplay = calloc(1, sizeof(struct display));
	r->mainDisplay->window = NULL;
	r->mainDisplay->renderer = NULL;
	r->mainDisplay->texture = NULL;
	r->mainDisplay->overlayTexture = NULL;
#else
	logr(warning, "Render preview is disabled. (No SDL2)\n");
#endif
	
	//Mutex
#ifdef _WIN32
	r->state.tileMutex = CreateMutex(NULL, FALSE, NULL);
#else
	r->state.tileMutex = (pthread_mutex_t)PTHREAD_MUTEX_INITIALIZER;
#endif
	return r;
}
	
void freeRenderer(struct renderer *r) {
	if (r->scene) {
		freeScene(r->scene);
		free(r->scene);
	}
	if (r->state.image) {
		freeTexture(r->state.image);
		free(r->state.image);
	}
	if (r->state.renderTiles) {
		free(r->state.renderTiles);
	}
	if (r->state.renderBuffer) {
		freeTexture(r->state.renderBuffer);
		free(r->state.renderBuffer);
	}
	if (r->state.uiBuffer) {
		freeTexture(r->state.uiBuffer);
		free(r->state.uiBuffer);
	}
	if (r->state.threadPaused) {
		free(r->state.threadPaused);
	}
	if (r->state.threadStates) {
		free(r->state.threadStates);
	}
#ifdef UI_ENABLED
	if (r->mainDisplay) {
		freeDisplay(r->mainDisplay);
		free(r->mainDisplay);
	}
#endif
	
	free(r);
}
