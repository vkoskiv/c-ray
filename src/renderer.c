//
//  renderer.c
//  C-Ray
//
//  Created by Valtteri Koskivuori on 19/02/2017.
//  Copyright © 2017 Valtteri Koskivuori. All rights reserved.
//

#include "includes.h"
#include "renderer.h"

#include "camera.h"
#include "scene.h"
#include "pathtrace.h"
#include "filehandler.h"
#include "main.h"
#include "logging.h"
#include "ui.h"
#include "tile.h"
#include "timer.h"

struct color getPixel(struct renderer *r, int x, int y);

void render(struct renderer *r) {
	logr(info, "Starting C-ray renderer for frame %i\n", r->image->count);
	
	logr(info, "Rendering at %i x %i\n", r->image->size.width,r->image->size.height);
	logr(info, "Rendering %i samples with %i bounces.\n", r->sampleCount, r->scene->bounces);
	logr(info, "Rendering with %d thread", r->threadCount);
	if (r->threadCount > 1) {
		printf("s.\n");
	} else {
		printf(".\n");
	}
	
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
			sleepMSec(100);
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
		
		//First time setup for each thread
		struct renderTile tile = getTile(renderer);
		
		while (tile.tileNum != -1 && renderer->isRendering) {
			startTimer(&renderer->timers[tinfo->thread_num]);
			
			while (tile.completedSamples < renderer->sampleCount+1 && renderer->isRendering) {
				for (int y = (int)tile.end.y; y > (int)tile.begin.y; y--) {
					for (int x = (int)tile.begin.x; x < (int)tile.end.x; x++) {
						
						int height = renderer->image->size.height;
						int width = renderer->image->size.width;
						
						double fracX = (double)x;
						double fracY = (double)y;
						
						//A cheap 'antialiasing' of sorts. The more samples, the better this works
						if (renderer->antialiasing) {
							fracX = getRandomDouble(fracX - 0.25, fracX + 0.25);
							fracY = getRandomDouble(fracY - 0.25, fracY + 0.25);
						}
						
						//Set up the light ray to be casted. direction is pointing towards the X,Y coordinate on the
						//imaginary plane in front of the origin. startPos is just the camera position.
						struct vector direction = {(fracX - 0.5 * renderer->image->size.width)
													/ renderer->scene->camera->focalLength,
												   (fracY - 0.5 * renderer->image->size.height)
													/ renderer->scene->camera->focalLength,
													1.0,
													false};
						
						//Normalize direction
						direction = normalizeVector(&direction);
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
							double randY = getRandomDouble(-aperture, aperture);
							double randX = getRandomDouble(-aperture, aperture);
							
							struct vector upTemp = vectorScale(randY, &up);
							struct vector temp = addVectors(&startPos, &upTemp);
							struct vector leftTemp = vectorScale(randX, &left);
							struct vector randomStart = addVectors(&temp, &leftTemp);
							
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
						struct color sample = pathTrace(&incidentRay, renderer->scene, 0);
						
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
				}
			}
			//Tile has finished rendering, get a new one and start rendering it.
			renderer->renderTiles[tile.tileNum].isRendering = false;
			unsigned long long samples = tile.completedSamples * (tile.width * tile.height);
			tile = getTile(renderer);
			unsigned long long duration = endTimer(&renderer->timers[tinfo->thread_num]);
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
	struct renderer *renderer = (struct renderer*)calloc(1, sizeof(struct renderer));
	renderer->avgTileTime = (time_t)1;
	renderer->timeSampleCount = 1;
	renderer->mode = saveModeNormal;
	renderer->image = (struct outputImage*)calloc(1, sizeof(struct outputImage));
	
	renderer->scene = (struct world*)calloc(1, sizeof(struct world));
	renderer->scene->camera = (struct camera*)calloc(1, sizeof(struct camera));
	renderer->scene->ambientColor = (struct color*)calloc(1, sizeof(struct color));
	renderer->scene->objs = (struct crayOBJ*)calloc(1, sizeof(struct crayOBJ));
	renderer->scene->spheres = (struct sphere*)calloc(1, sizeof(struct sphere));
	renderer->scene->materials = (struct material*)calloc(1, sizeof(struct material));
	
#ifdef UI_ENABLED
	renderer->mainDisplay = (struct display*)calloc(1, sizeof(struct display));
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
	output.red = r->renderBuffer[(x + (r->image->size.height - y) * r->image->size.width)*3 + 0];
	output.green = r->renderBuffer[(x + (r->image->size.height - y) * r->image->size.width)*3 + 1];
	output.blue = r->renderBuffer[(x + (r->image->size.height - y) * r->image->size.width)*3 + 2];
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
