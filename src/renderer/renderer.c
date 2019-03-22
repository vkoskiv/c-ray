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
#include "../utils/filehandler.h"
#include "../main.h"
#include "../utils/logging.h"
#include "../utils/ui.h"
#include "../datatypes/tile.h"
#include "../utils/timer.h"

struct color getPixel(struct renderer *r, int x, int y);

void render(struct renderer *r) {
	logr(info, "Starting C-ray renderer for frame %i\n", r->image->count);
	
	logr(info, "Rendering at %i x %i\n", *r->image->width,*r->image->height);
	logr(info, "Rendering %i samples with %i bounces.\n", r->sampleCount, r->scene->bounces);
	logr(info, "Rendering with %d thread", r->threadCount);
	printf(r->threadCount > 1 ? "s.\n" : ".\n");
	
	logr(info, "Pathtracing...\n");
	
	//Create threads
	int t;
	
	r->isRendering = true;
	r->renderAborted = false;
#ifndef WINDOWS
	pthread_attr_init(&r->renderThreadAttributes);
	pthread_attr_setdetachstate(&r->renderThreadAttributes, PTHREAD_CREATE_JOINABLE);
#endif
	//Main loop (input)
	bool threadsHaveStarted = false;
	while (r->isRendering) {
#ifdef UI_ENABLED
		getKeyboardInput(r);
		drawWindow(r);
#endif
		
		if (!threadsHaveStarted) {
			threadsHaveStarted = true;
			//Create render threads
			for (t = 0; t < r->threadCount; t++) {
				r->renderThreadInfo[t].thread_num = t;
				r->renderThreadInfo[t].threadComplete = false;
				r->renderThreadInfo[t].r = r;
				r->activeThreads++;
#ifdef WINDOWS
				DWORD threadId;
				r->renderThreadInfo[t].thread_handle = CreateThread(NULL, 0, renderThread, &r->renderThreadInfo[t], 0, &threadId);
				if (r->renderThreadInfo[t].thread_handle == NULL) {
					logr(error, "Failed to create thread.\n");
					exit(-1);
				}
				r->renderThreadInfo[t].thread_id = threadId;
#else
				if (pthread_create(&r->renderThreadInfo[t].thread_id, &r->renderThreadAttributes, renderThread, &r->renderThreadInfo[t])) {
					logr(error, "Failed to create a thread.\n");
				}
#endif
			}
			
			r->renderThreadInfo->threadComplete = false;
			
#ifndef WINDOWS
			if (pthread_attr_destroy(&r->renderThreadAttributes)) {
				logr(warning, "Failed to destroy pthread.\n");
			}
#endif
		}
		
		//Wait for render threads to finish (Render finished)
		for (t = 0; t < r->threadCount; t++) {
			if (r->renderThreadInfo[t].threadComplete && r->renderThreadInfo[t].thread_num != -1) {
				r->activeThreads--;
				r->renderThreadInfo[t].thread_num = -1;
			}
			if (r->activeThreads == 0 || r->renderAborted) {
				r->isRendering = false;
			}
		}
		if (r->threadPaused[0]) {
			sleepMSec(800);
		} else {
			sleepMSec(16);
		}
	}
	
	//Make sure render threads are finished before continuing
	for (t = 0; t < r->threadCount; t++) {
#ifdef WINDOWS
		WaitForSingleObjectEx(r->renderThreadInfo[t].thread_handle, INFINITE, FALSE);
#else
		if (pthread_join(r->renderThreadInfo[t].thread_id, NULL)) {
			logr(warning, "Thread %t frozen.", t);
		}
#endif
	}
}

/**
 A global render thread
 
 @param arg Thread information (see threadInfo struct)
 @return Exits when thread is done
 */
#ifdef WINDOWS
DWORD WINAPI renderThreadGlobal(LPVOID arg) {
#else
	void *renderThreadGlobal(void *arg) {
#endif
		struct lightRay incidentRay;
		struct threadInfo *tinfo = (struct threadInfo*)arg;
		
		struct renderer *renderer = tinfo->r;
		pcg32_random_t *rng = &tinfo->r->rngs[tinfo->thread_num];
		
		//We keep track of milliseconds spent sleeping, and subtract that from the total.
		unsigned long long sleepMs = 0;
		startTimer(&renderer->timers[tinfo->thread_num]);
		
		int completedSamples = 0;
		int height = *renderer->image->height;
		int width = *renderer->image->width;
		
		while (completedSamples < renderer->sampleCount+1 && renderer->isRendering) {
			for (int y = 0; y < *renderer->image->height-1; y++) {
				for (int x = 0; x < ((*renderer->image->width-1)); x += renderer->threadCount) {
					int tempx = x + tinfo->thread_num;
					if (tempx > *renderer->image->width) break;
					
					double fracX = (double)tempx;
					double fracY = (double)y;
					
					//A cheap 'antialiasing' of sorts. The more samples, the better this works
					if (renderer->antialiasing) {
						fracX = rndDouble(fracX - 0.25, fracX + 0.25, rng);
						fracY = rndDouble(fracY - 0.25, fracY + 0.25, rng);
					}
					
					//Set up the light ray to be casted. direction is pointing towards the X,Y coordinate on the
					//imaginary plane in front of the origin. startPos is just the camera position.
					struct vector direction = {(fracX - 0.5 * *renderer->image->width)
						/ renderer->scene->camera->focalLength,
						(fracY - 0.5 * *renderer->image->height)
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
					incidentRay.remainingInteractions = renderer->scene->bounces;
					incidentRay.currentMedium.IOR = AIR_IOR;
					
					//For multi-sample rendering, we keep a running average of color values for each pixel
					//The next block of code does this
					
					//Get previous color value from render buffer
					struct color output = getPixel(renderer, tempx, y);
					
					//Get new sample (path tracing is initiated here)
					struct color sample = pathTrace(&incidentRay, renderer->scene, 0, rng);
					
					//And process the running average
					output.red = output.red * (completedSamples - 1);
					output.green = output.green * (completedSamples - 1);
					output.blue = output.blue * (completedSamples - 1);
					
					output = addColors(&output, &sample);
					
					output.red = output.red / completedSamples;
					output.green = output.green / completedSamples;
					output.blue = output.blue / completedSamples;
					
					//Store render buffer
					renderer->renderBuffer[(tempx + (height - y)*width)*3 + 0] = output.red;
					renderer->renderBuffer[(tempx + (height - y)*width)*3 + 1] = output.green;
					renderer->renderBuffer[(tempx + (height - y)*width)*3 + 2] = output.blue;
					
					//Gamma correction
					output = toSRGB(output);
					
					//And store the image data
					//Note how imageData only stores 8-bit precision for each color channel.
					//This is why we use the renderBuffer for the running average as it just contains
					//the full precision color values
					renderer->image->data[(tempx + (height - y)*width)*3 + 0] =
					(unsigned char)min( max(output.red*255.0,0), 255.0);
					renderer->image->data[(tempx + (height - y)*width)*3 + 1] =
					(unsigned char)min( max(output.green*255.0,0), 255.0);
					renderer->image->data[(tempx + (height - y)*width)*3 + 2] =
					(unsigned char)min( max(output.blue*255.0,0), 255.0);
				}
			}
			completedSamples++;
			//Pause rendering when bool is set
			while (renderer->threadPaused[tinfo->thread_num] && !renderer->renderAborted) {
				sleepMSec(100);
				sleepMs += 100;
			}
		}
		//Tile has finished rendering, get a new one and start rendering it.
		unsigned long long samples = completedSamples * (*renderer->image->width * *renderer->image->height);
		unsigned long long duration = endTimer(&renderer->timers[tinfo->thread_num]);
		if (sleepMs > 0) {
			duration -= sleepMs;
		}
		printStats(renderer, duration, samples, tinfo->thread_num);
		
		//Max samples reached, exit thread. (render done)
		printf("%s", "\33[2K");
		logr(info, "Thread %i done\n", tinfo->thread_num);
		tinfo->threadComplete = true;
#ifdef WINDOWS
		return 0;
#else
		pthread_exit((void*) arg);
#endif
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
		pcg32_random_t *rng = &tinfo->r->rngs[tinfo->thread_num];
		
		//First time setup for each thread
		struct renderTile tile = getTile(renderer);
		
		while (tile.tileNum != -1 && renderer->isRendering) {
			unsigned long long sleepMs = 0;
			startTimer(&renderer->timers[tinfo->thread_num]);
			
			while (tile.completedSamples < renderer->sampleCount+1 && renderer->isRendering) {
				for (int y = (int)tile.end.y; y > (int)tile.begin.y; y--) {
					for (int x = (int)tile.begin.x; x < (int)tile.end.x; x++) {
						
						int height = *renderer->image->height;
						int width = *renderer->image->width;
						
						double fracX = (double)x;
						double fracY = (double)y;
						
						//A cheap 'antialiasing' of sorts. The more samples, the better this works
						if (renderer->antialiasing) {
							fracX = rndDouble(fracX - 0.25, fracX + 0.25, rng);
							fracY = rndDouble(fracY - 0.25, fracY + 0.25, rng);
						}
						
						//Set up the light ray to be casted. direction is pointing towards the X,Y coordinate on the
						//imaginary plane in front of the origin. startPos is just the camera position.
						struct vector direction = {(fracX - 0.5 * *renderer->image->width)
													/ renderer->scene->camera->focalLength,
												   (fracY - 0.5 * *renderer->image->height)
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
						incidentRay.remainingInteractions = renderer->scene->bounces;
						incidentRay.currentMedium.IOR = AIR_IOR;
						
						//For multi-sample rendering, we keep a running average of color values for each pixel
						//The next block of code does this
						
						//Get previous color value from render buffer
						struct color output = getPixel(renderer, x, y);
						
						//Get new sample (path tracing is initiated here)
						struct color sample = pathTrace(&incidentRay, renderer->scene, 0, rng);
						
						//And process the running average
						output.red = output.red * (tile.completedSamples - 1);
						output.green = output.green * (tile.completedSamples - 1);
						output.blue = output.blue * (tile.completedSamples - 1);
						
						output = addColors(&output, &sample);
						
						output.red = output.red / tile.completedSamples;
						output.green = output.green / tile.completedSamples;
						output.blue = output.blue / tile.completedSamples;
						
						//Store render buffer
						renderer->renderBuffer[(x + (height - y)*width)*3 + 0] = output.red;
						renderer->renderBuffer[(x + (height - y)*width)*3 + 1] = output.green;
						renderer->renderBuffer[(x + (height - y)*width)*3 + 2] = output.blue;
						
						//Gamma correction
						output = toSRGB(output);
						
						//And store the image data
						//Note how imageData only stores 8-bit precision for each color channel.
						//This is why we use the renderBuffer for the running average as it just contains
						//the full precision color values
						renderer->image->data[(x + (height - y)*width)*3 + 0] =
						(unsigned char)min( max(output.red*255.0,0), 255.0);
						renderer->image->data[(x + (height - y)*width)*3 + 1] =
						(unsigned char)min( max(output.green*255.0,0), 255.0);
						renderer->image->data[(x + (height - y)*width)*3 + 2] =
						(unsigned char)min( max(output.blue*255.0,0), 255.0);
					}
				}
				tile.completedSamples++;
				//Pause rendering when bool is set
				while (renderer->threadPaused[tinfo->thread_num] && !renderer->renderAborted) {
					sleepMSec(100);
					sleepMs += 100;
				}
			}
			//Tile has finished rendering, get a new one and start rendering it.
			renderer->renderTiles[tile.tileNum].isRendering = false;
			unsigned long long samples = tile.completedSamples * (tile.width * tile.height);
			tile = getTile(renderer);
			unsigned long long duration = endTimer(&renderer->timers[tinfo->thread_num]);
			if (sleepMs > 0) {
				duration -= sleepMs;
			}
			printStats(renderer, duration, samples, tinfo->thread_num);
		}
		//No more tiles to render, exit thread. (render done)
		printf("%s", "\33[2K");
		logr(info, "Thread %i done\n", tinfo->thread_num);
		tinfo->threadComplete = true;
#ifdef WINDOWS
		return 0;
#else
		pthread_exit((void*) arg);
#endif
}
	
struct renderer *newRenderer() {
	struct renderer *renderer = calloc(1, sizeof(struct renderer));
	renderer->avgTileTime = (time_t)1;
	renderer->timeSampleCount = 1;
	renderer->mode = saveModeNormal;
	renderer->image = calloc(1, sizeof(struct texture));
	renderer->image->width = calloc(1, sizeof(unsigned int));
	renderer->image->height = calloc(1, sizeof(unsigned int));
	
	renderer->scene = calloc(1, sizeof(struct world));
	renderer->scene->camera = calloc(1, sizeof(struct camera));
	renderer->scene->ambientColor = calloc(1, sizeof(struct color));
	renderer->scene->objs = calloc(1, sizeof(struct crayOBJ));
	renderer->scene->spheres = calloc(1, sizeof(struct sphere));
	renderer->scene->materials = calloc(1, sizeof(struct material));
	
#ifdef UI_ENABLED
	renderer->mainDisplay = calloc(1, sizeof(struct display));
	renderer->mainDisplay->window = NULL;
	renderer->mainDisplay->renderer = NULL;
	renderer->mainDisplay->texture = NULL;
	renderer->mainDisplay->overlayTexture = NULL;
#else
	printf("**************************************************************************\n");
	printf("*      UI is DISABLED! Enable by installing SDL2 and doing `cmake .`     *\n");
	printf("**************************************************************************\n");
#endif
	
	//Mutex
#ifdef _WIN32
	renderer->tileMutex = CreateMutex(NULL, FALSE, NULL);
#else
	renderer->tileMutex = (pthread_mutex_t)PTHREAD_MUTEX_INITIALIZER;
#endif
	return renderer;
}
	
//TODO: Refactor this to retrieve pixel from a given buffer, so we can reuse it for texture maps
struct color getPixel(struct renderer *r, int x, int y) {
	struct color output = {0.0, 0.0, 0.0, 0.0};
	output.red = r->renderBuffer[(x + (*r->image->height - y) * *r->image->width)*3 + 0];
	output.green = r->renderBuffer[(x + (*r->image->height - y) * *r->image->width)*3 + 1];
	output.blue = r->renderBuffer[(x + (*r->image->height - y) * *r->image->width)*3 + 2];
	output.alpha = 1.0;
	return output;
}
	
void freeRenderer(struct renderer *r) {
	if (r->scene) {
		freeScene(r->scene);
		free(r->scene);
	}
	if (r->image) {
		freeImage(r->image);
		free(r->image);
	}
	if (r->renderTiles) {
		free(r->renderTiles);
	}
	if (r->renderBuffer) {
		free(r->renderBuffer);
	}
	if (r->uiBuffer) {
		free(r->uiBuffer);
	}
	if (r->threadPaused) {
		free(r->threadPaused);
	}
	if (r->renderThreadInfo) {
		free(r->renderThreadInfo);
	}
#ifdef UI_ENABLED
	if (r->mainDisplay) {
		freeDisplay(r->mainDisplay);
	}
#endif
	if (r->timers) {
		free(r->timers);
	}
	if (r->inputFilePath) {
		free(r->inputFilePath);
	}
	
	free(r);
}
