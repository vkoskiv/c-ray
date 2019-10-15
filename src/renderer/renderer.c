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

struct color getPixel(struct renderer *r, int x, int y);

/// @todo Use defaultSettings state struct for this.
/// @todo Clean this up, it's ugly.
void render(struct renderer *r) {
	logr(info, "Starting C-ray renderer for frame %i\n", r->state.image->count);
	
	logr(info, "Rendering at %i x %i\n", *r->state.image->width,*r->state.image->height);
	logr(info, "Rendering %i samples with %i bounces.\n", r->prefs.sampleCount, r->prefs.bounces);
	logr(info, "Rendering with %d thread", r->prefs.threadCount);
	printf(r->prefs.threadCount > 1 ? "s.\n" : ".\n");
	
	logr(info, "Pathtracing...\n");
	
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
	while (r->state.isRendering) {
#ifdef UI_ENABLED
		getKeyboardInput(r);
		drawWindow(r);
#endif
		
		if (!threadsHaveStarted) {
			threadsHaveStarted = true;
			//Create render threads
			for (t = 0; t < r->prefs.threadCount; t++) {
				r->state.renderThreadInfo[t].thread_num = t;
				r->state.renderThreadInfo[t].threadComplete = false;
				r->state.renderThreadInfo[t].r = r;
				r->state.activeThreads++;
#ifdef WINDOWS
				DWORD threadId;
				r->state.renderThreadInfo[t].thread_handle = CreateThread(NULL, 0, renderThread, &r->state.renderThreadInfo[t], 0, &threadId);
				if (r->state.renderThreadInfo[t].thread_handle == NULL) {
					logr(error, "Failed to create thread.\n");
					exit(-1);
				}
				r->state.renderThreadInfo[t].thread_id = threadId;
#else
				if (pthread_create(&r->state.renderThreadInfo[t].thread_id, &r->state.renderThreadAttributes, renderThread, &r->state.renderThreadInfo[t])) {
					logr(error, "Failed to create a thread.\n");
				}
#endif
			}
			
			r->state.renderThreadInfo->threadComplete = false;
			
#ifndef WINDOWS
			if (pthread_attr_destroy(&r->state.renderThreadAttributes)) {
				logr(warning, "Failed to destroy pthread.\n");
			}
#endif
		}
		
		//Wait for render threads to finish (Render finished)
		for (t = 0; t < r->prefs.threadCount; t++) {
			if (r->state.renderThreadInfo[t].threadComplete && r->state.renderThreadInfo[t].thread_num != -1) {
				r->state.activeThreads--;
				r->state.renderThreadInfo[t].thread_num = -1;
			}
			if (r->state.activeThreads == 0 || r->state.renderAborted) {
				r->state.isRendering = false;
			}
		}
		if (r->state.threadPaused[0]) {
			sleepMSec(800);
		} else {
			sleepMSec(40);
		}
	}
	
	//Make sure render threads are finished before continuing
	for (t = 0; t < r->prefs.threadCount; t++) {
#ifdef WINDOWS
		WaitForSingleObjectEx(r->state.renderThreadInfo[t].thread_handle, INFINITE, FALSE);
#else
		if (pthread_join(r->state.renderThreadInfo[t].thread_id, NULL)) {
			logr(warning, "Thread %t frozen.", t);
		}
#endif
	}
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
	struct threadInfo *tinfo = (struct threadInfo*)arg;
	
	struct renderer *renderer = tinfo->r;
	pcg32_random_t *rng = &tinfo->r->state.rngs[tinfo->thread_num];
	
	//First time setup for each thread
	struct renderTile tile = getTile(renderer);
	tinfo->currentTileNum = tile.tileNum;
	
	bool hasHitObject = false;
	
	while (tile.tileNum != -1 && renderer->state.isRendering) {
		unsigned long long sleepMs = 0;
		startTimer(&renderer->state.timers[tinfo->thread_num]);
		hasHitObject = false;
		
		while (tile.completedSamples < renderer->prefs.sampleCount+1 && renderer->state.isRendering) {
			for (int y = (int)tile.end.y; y > (int)tile.begin.y; y--) {
				for (int x = (int)tile.begin.x; x < (int)tile.end.x; x++) {
					
					int height = *renderer->state.image->height;
					int width = *renderer->state.image->width;
					
					double fracX = (double)x;
					double fracY = (double)y;
					
					//A cheap 'antialiasing' of sorts. The more samples, the better this works
					float jitter = 0.25;
					if (renderer->prefs.antialiasing) {
						fracX = rndDouble(fracX - jitter, fracX + jitter, rng);
						fracY = rndDouble(fracY - jitter, fracY + jitter, rng);
					}
					
					//Set up the light ray to be casted. direction is pointing towards the X,Y coordinate on the
					//imaginary plane in front of the origin. startPos is just the camera position.
					struct vector direction = {(fracX - 0.5 * *renderer->state.image->width)
												/ renderer->scene->camera->focalLength,
											   (fracY - 0.5 * *renderer->state.image->height)
												/ renderer->scene->camera->focalLength,
												1.0};
					
					//Normalize direction
					direction = vecNormalize(&direction);
					struct vector startPos = renderer->scene->camera->pos;
					struct vector left = renderer->scene->camera->left;
					struct vector up = renderer->scene->camera->up;
					
					//Run camera tranforms on direction vector
					transformCameraView(renderer->scene->camera, &direction);
					
					//Now handle aperture
					//FIXME: This is a 'square' aperture
					double aperture = renderer->scene->camera->aperture;
					if (aperture <= 0.0) {
						incidentRay.start = startPos;
					} else {
						double randY = rndDouble(-aperture, aperture, rng);
						double randX = rndDouble(-aperture, aperture, rng);
						
						struct vector upTemp = vecScale(randY, &up);
						struct vector temp = vecAdd(&startPos, &upTemp);
						struct vector leftTemp = vecScale(randX, &left);
						struct vector randomStart = vecAdd(&temp, &leftTemp);
						
						incidentRay.start = randomStart;
					}
					
					incidentRay.direction = direction;
					incidentRay.rayType = rayTypeIncident;
					incidentRay.remainingInteractions = renderer->prefs.bounces;
					incidentRay.currentMedium.IOR = AIR_IOR;
					
					//For multi-sample rendering, we keep a running average of color values for each pixel
					//The next block of code does this
					
					//Get previous color value from render buffer
					struct color output = getPixel(renderer, x, y);
					
					//Get new sample (path tracing is initiated here)
					struct color sample = pathTrace(&incidentRay, renderer->scene, 0, renderer->prefs.bounces, rng, &hasHitObject);
					
					//And process the running average
					output.red = output.red * (tile.completedSamples - 1);
					output.green = output.green * (tile.completedSamples - 1);
					output.blue = output.blue * (tile.completedSamples - 1);
					
					output = addColors(&output, &sample);
					
					output.red = output.red / tile.completedSamples;
					output.green = output.green / tile.completedSamples;
					output.blue = output.blue / tile.completedSamples;
					
					//Store internal render buffer (double precision)
					blitDouble(renderer->state.renderBuffer, width, height, &output, x, y);
					
					//Gamma correction
					output = toSRGB(output);
					
					//And store the image data
					blit(renderer->state.image, output, x, y);
				}
			}
			tile.completedSamples++;
			tinfo->completedSamples = tile.completedSamples;
			if (tile.completedSamples > 25 && !hasHitObject) break; //Abort if we didn't hit anything within 25 samples
			//Pause rendering when bool is set
			while (renderer->state.threadPaused[tinfo->thread_num] && !renderer->state.renderAborted) {
				sleepMSec(100);
				sleepMs += 100;
			}
		}
		//Tile has finished rendering, get a new one and start rendering it.
		renderer->state.renderTiles[tile.tileNum].isRendering = false;
		renderer->state.renderTiles[tile.tileNum].renderComplete = true;
		tinfo->currentTileNum = -1;
		tinfo->completedSamples = 0;
		unsigned long long samples = tile.completedSamples * (tile.width * tile.height);
		tile = getTile(renderer);
		tinfo->currentTileNum = tile.tileNum;
		unsigned long long duration = endTimer(&renderer->state.timers[tinfo->thread_num]);
		if (sleepMs > 0) {
			duration -= sleepMs;
		}
		printStats(renderer, duration, samples, tinfo->thread_num);
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
	struct renderer *renderer = calloc(1, sizeof(struct renderer));
	renderer->state.avgTileTime = (time_t)1;
	renderer->state.timeSampleCount = 1;
	renderer->prefs.fileMode = saveModeNormal;
	renderer->state.image = calloc(1, sizeof(struct texture));
	renderer->state.image->width = calloc(1, sizeof(unsigned int));
	renderer->state.image->height = calloc(1, sizeof(unsigned int));
	
	renderer->scene = calloc(1, sizeof(struct world));
	renderer->scene->camera = calloc(1, sizeof(struct camera));
	renderer->scene->ambientColor = calloc(1, sizeof(struct color));
	renderer->scene->hdr = NULL; //Optional, to be loaded later
	renderer->scene->meshes = calloc(1, sizeof(struct mesh));
	renderer->scene->spheres = calloc(1, sizeof(struct sphere));
	
#ifdef UI_ENABLED
	renderer->mainDisplay = calloc(1, sizeof(struct display));
	renderer->mainDisplay->window = NULL;
	renderer->mainDisplay->renderer = NULL;
	renderer->mainDisplay->texture = NULL;
	renderer->mainDisplay->overlayTexture = NULL;
#else
	logr(warning, "Render preview is disabled. (No SDL2)\n");
#endif
	
	//Mutex
#ifdef _WIN32
	renderer->state.tileMutex = CreateMutex(NULL, FALSE, NULL);
#else
	renderer->state.tileMutex = (pthread_mutex_t)PTHREAD_MUTEX_INITIALIZER;
#endif
	return renderer;
}
	
//TODO: Refactor this to retrieve pixel from a given buffer, so we can reuse it for texture maps
struct color getPixel(struct renderer *r, int x, int y) {
	struct color output = {0.0, 0.0, 0.0, 0.0};
	output.red = r->state.renderBuffer[(x + (*r->state.image->height - y) * *r->state.image->width)*3 + 0];
	output.green = r->state.renderBuffer[(x + (*r->state.image->height - y) * *r->state.image->width)*3 + 1];
	output.blue = r->state.renderBuffer[(x + (*r->state.image->height - y) * *r->state.image->width)*3 + 2];
	output.alpha = 1.0;
	return output;
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
		free(r->state.renderBuffer);
	}
	if (r->state.uiBuffer) {
		free(r->state.uiBuffer);
	}
	if (r->state.threadPaused) {
		free(r->state.threadPaused);
	}
	if (r->state.renderThreadInfo) {
		free(r->state.renderThreadInfo);
	}
#ifdef UI_ENABLED
	if (r->mainDisplay) {
		freeDisplay(r->mainDisplay);
	}
#endif
	if (r->state.timers) {
		free(r->state.timers);
	}
	
	free(r);
}
