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

void render(struct renderer *r) {
	logr(info, "Starting C-ray renderer for frame %i\n", r->image->count);
	
	logr(info, "Rendering at %i x %i\n", *r->image->width,*r->image->height);
	logr(info, "Rendering %i samples with %i bounces.\n", r->sampleCount, r->bounces);
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
			sleepMSec(40);
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
 A SIMD-optimised render thread
 
 @param arg Thread information (see threadInfo struct)
 @return Exits when thread is done
 */
#ifdef WINDOWS
DWORD WINAPI renderThreadSIMD(LPVOID arg) {
#else
void *renderThreadSIMD(void *arg) {
#endif
	struct threadInfo *tinfo = (struct threadInfo*)arg;
	
	struct renderer *renderer = tinfo->r;
	pcg32_random_t *rng = &tinfo->r->rngs[tinfo->thread_num];
	
	//First time setup for each thread
	struct renderTile tile = getTile(renderer);
	
	int height = *renderer->image->height;
	int width = *renderer->image->width;
	
	struct vector startPos = renderer->scene->camera->pos;
	
	struct lightRay **incidentRay = malloc(width * sizeof(struct lightRay*));
	for (int i = 0; i < width; i++) {
		incidentRay[i] = malloc(height * sizeof(struct lightRay));
	}
	
	double **fracX = malloc(width * sizeof(double*));
	for (int i = 0; i < width; i++) {
		fracX[i] = malloc(height * sizeof(double));
	}
	double **fracY = malloc(width * sizeof(double*));
	for (int i = 0; i < width; i++) {
		fracY[i] = malloc(height * sizeof(double));
	}
	struct vector **direction = malloc(width * sizeof(struct vector*));
	for (int i = 0; i < width; i++) {
		direction[i] = malloc(height * sizeof(struct vector));
	}
	struct color **output = malloc(width * sizeof(struct color*));
	for (int i = 0; i < width; i++) {
		output[i] = malloc(height * sizeof(struct color));
	}
	struct color **sample = malloc(width * sizeof(struct color*));
	for (int i = 0; i < width; i++) {
		sample[i] = malloc(height * sizeof(struct color));
	}
	
	while (tile.tileNum != -1 && renderer->isRendering) {
		unsigned long long sleepMs = 0;
		startTimer(&renderer->timers[tinfo->thread_num]);
		
		while (tile.completedSamples < renderer->sampleCount+1 && renderer->isRendering) {
			
			//VECD indicates 'basic block vectorized' reported by gcc-8
			
			for (int y = (int)tile.end.y; y > (int)tile.begin.y; y--) {
				for (int x = (int)tile.begin.x; x < (int)tile.end.x; x++) {
					int tx = x - (int)tile.begin.x;
					int ty = y - (int)tile.begin.y;
					fracX[tx][ty] = (double)x;
					fracY[tx][ty] = (double)y;
				}
			}
			
			if (renderer->antialiasing) {
				for (int y = (int)tile.end.y; y > (int)tile.begin.y; y--) {
					for (int x = (int)tile.begin.x; x < (int)tile.end.x; x++) {
						int tx = x - (int)tile.begin.x;
						int ty = y - (int)tile.begin.y;
						fracX[tx][ty] = rndDouble(fracX[tx][ty] - 0.25, fracX[tx][ty] + 0.25, rng);
						fracY[tx][ty] = rndDouble(fracY[tx][ty] - 0.25, fracY[tx][ty] + 0.25, rng);
					}
				}
			}
			
			for (int y = (int)tile.end.y; y > (int)tile.begin.y; y--) {
				for (int x = (int)tile.begin.x; x < (int)tile.end.x; x++) { //VECD
					int tx = x - (int)tile.begin.x;
					int ty = y - (int)tile.begin.y;
					direction[tx][ty] = (struct vector){(fracX[tx][ty] - 0.5 * width)
						/ renderer->scene->camera->focalLength,
						(fracY[tx][ty] - 0.5 * height)
						/ renderer->scene->camera->focalLength,
						1.0};
				}
			}
			
			for (int y = (int)tile.end.y; y > (int)tile.begin.y; y--) {
				for (int x = (int)tile.begin.x; x < (int)tile.end.x; x++) {
					int tx = x - (int)tile.begin.x;
					int ty = y - (int)tile.begin.y;
					direction[tx][ty] = vecNormalize(&direction[tx][ty]);
				}
			}
			
			for (int y = (int)tile.end.y; y > (int)tile.begin.y; y--) {
				for (int x = (int)tile.begin.x; x < (int)tile.end.x; x++) {
					int tx = x - (int)tile.begin.x;
					int ty = y - (int)tile.begin.y;
					transformCameraView(renderer->scene->camera, &direction[tx][ty]);
				}
			}
			
			for (int y = (int)tile.end.y; y > (int)tile.begin.y; y--) {
				for (int x = (int)tile.begin.x; x < (int)tile.end.x; x++) { //VECD
					int tx = x - (int)tile.begin.x;
					int ty = y - (int)tile.begin.y;
					incidentRay[tx][ty].start = startPos;
					incidentRay[tx][ty].direction = direction[tx][ty];
					incidentRay[tx][ty].rayType = rayTypeIncident;
					incidentRay[tx][ty].remainingInteractions = renderer->bounces;
					incidentRay[tx][ty].currentMedium.IOR = AIR_IOR;
				}
			}
			
			for (int y = (int)tile.end.y; y > (int)tile.begin.y; y--) {
				for (int x = (int)tile.begin.x; x < (int)tile.end.x; x++) { //VECD
					int tx = x - (int)tile.begin.x;
					int ty = y - (int)tile.begin.y;
					output[tx][ty] = getPixel(renderer, x, y);
				}
			}
			
			for (int y = (int)tile.end.y; y > (int)tile.begin.y; y--) {
				for (int x = (int)tile.begin.x; x < (int)tile.end.x; x++) {
					int tx = x - (int)tile.begin.x;
					int ty = y - (int)tile.begin.y;
					sample[tx][ty] = pathTrace(&incidentRay[tx][ty], renderer->scene, 0, renderer->bounces, rng);
				}
			}
			
			for (int y = (int)tile.end.y; y > (int)tile.begin.y; y--) {
				for (int x = (int)tile.begin.x; x < (int)tile.end.x; x++) { //VECD
					int tx = x - (int)tile.begin.x;
					int ty = y - (int)tile.begin.y;
					output[tx][ty].red = output[tx][ty].red * (tile.completedSamples - 1);
					output[tx][ty].green = output[tx][ty].green * (tile.completedSamples - 1);
					output[tx][ty].blue = output[tx][ty].blue * (tile.completedSamples - 1);
				}
			}
			
			for (int y = (int)tile.end.y; y > (int)tile.begin.y; y--) {
				for (int x = (int)tile.begin.x; x < (int)tile.end.x; x++) {
					int tx = x - (int)tile.begin.x;
					int ty = y - (int)tile.begin.y;
					output[tx][ty] = addColors(&output[tx][ty], &sample[tx][ty]);
				}
			}
			
			for (int y = (int)tile.end.y; y > (int)tile.begin.y; y--) {
				for (int x = (int)tile.begin.x; x < (int)tile.end.x; x++) { //VECD
					int tx = x - (int)tile.begin.x;
					int ty = y - (int)tile.begin.y;
					output[tx][ty].red = output[tx][ty].red / tile.completedSamples;
					output[tx][ty].green = output[tx][ty].green / tile.completedSamples;
					output[tx][ty].blue = output[tx][ty].blue / tile.completedSamples;
				}
			}
			
			for (int y = (int)tile.end.y; y > (int)tile.begin.y; y--) {
				for (int x = (int)tile.begin.x; x < (int)tile.end.x; x++) {
					int tx = x - (int)tile.begin.x;
					int ty = y - (int)tile.begin.y;
					blitDouble(renderer->renderBuffer, width, height, &output[tx][ty], x, y);
				}
			}
			
			for (int y = (int)tile.end.y; y > (int)tile.begin.y; y--) {
				for (int x = (int)tile.begin.x; x < (int)tile.end.x; x++) {
					int tx = x - (int)tile.begin.x;
					int ty = y - (int)tile.begin.y;
					output[tx][ty] = toSRGB(output[tx][ty]);
				}
			}
			
			for (int y = (int)tile.end.y; y > (int)tile.begin.y; y--) {
				for (int x = (int)tile.begin.x; x < (int)tile.end.x; x++) {
					int tx = x - (int)tile.begin.x;
					int ty = y - (int)tile.begin.y;
					blit(renderer->image, output[tx][ty], x, y);
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
	
	/*for (int i = 0; i < width; i++) {
		free(incidentRay[i]);
	}
	free(incidentRay);
	for (int i = 0; i < width; i++) {
		free(fracX[i]);
	}
	free(fracX);
	for (int i = 0; i < width; i++) {
		free(fracY[i]);
	}
	free(fracY);
	for (int i = 0; i < width; i++) {
		free(direction[i]);
	}
	free(direction);
	for (int i = 0; i < width; i++) {
		free(output[i]);
	}
	free(output);
	for (int i = 0; i < width; i++) {
		free(sample[i]);
	}
	free(sample);*/
	
	//No more tiles to render, exit thread. (render done)
	printf("%s", "\33[2K");
	logr(info, "Thread %i done\r", tinfo->thread_num);
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
	tinfo->currentTileNum = tile.tileNum;
	
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
					float jitter = 0.25;
					if (renderer->antialiasing) {
						fracX = rndDouble(fracX - jitter, fracX + jitter, rng);
						fracY = rndDouble(fracY - jitter, fracY + jitter, rng);
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
					incidentRay.remainingInteractions = renderer->bounces;
					incidentRay.currentMedium.IOR = AIR_IOR;
					
					//For multi-sample rendering, we keep a running average of color values for each pixel
					//The next block of code does this
					
					//Get previous color value from render buffer
					struct color output = getPixel(renderer, x, y);
					
					//Get new sample (path tracing is initiated here)
					struct color sample = pathTrace(&incidentRay, renderer->scene, 0, renderer->bounces, rng);
					
					//And process the running average
					output.red = output.red * (tile.completedSamples - 1);
					output.green = output.green * (tile.completedSamples - 1);
					output.blue = output.blue * (tile.completedSamples - 1);
					
					output = addColors(&output, &sample);
					
					output.red = output.red / tile.completedSamples;
					output.green = output.green / tile.completedSamples;
					output.blue = output.blue / tile.completedSamples;
					
					//Store internal render buffer (double precision)
					blitDouble(renderer->renderBuffer, width, height, &output, x, y);
					
					//Gamma correction
					output = toSRGB(output);
					
					//And store the image data
					blit(renderer->image, output, x, y);
				}
			}
			tile.completedSamples++;
			tinfo->completedSamples = tile.completedSamples;
			//Pause rendering when bool is set
			while (renderer->threadPaused[tinfo->thread_num] && !renderer->renderAborted) {
				sleepMSec(100);
				sleepMs += 100;
			}
		}
		//Tile has finished rendering, get a new one and start rendering it.
		renderer->renderTiles[tile.tileNum].isRendering = false;
		renderer->renderTiles[tile.tileNum].renderComplete = true;
		tinfo->currentTileNum = -1;
		tinfo->completedSamples = 0;
		unsigned long long samples = tile.completedSamples * (tile.width * tile.height);
		tile = getTile(renderer);
		tinfo->currentTileNum = tile.tileNum;
		unsigned long long duration = endTimer(&renderer->timers[tinfo->thread_num]);
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
	renderer->avgTileTime = (time_t)1;
	renderer->timeSampleCount = 1;
	renderer->mode = saveModeNormal;
	renderer->image = calloc(1, sizeof(struct texture));
	renderer->image->width = calloc(1, sizeof(unsigned int));
	renderer->image->height = calloc(1, sizeof(unsigned int));
	
	renderer->scene = calloc(1, sizeof(struct world));
	renderer->scene->camera = calloc(1, sizeof(struct camera));
	renderer->scene->ambientColor = calloc(1, sizeof(struct color));
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
		freeTexture(r->image);
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
