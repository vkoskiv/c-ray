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
#include "light.h"
#include "poly.h"
#include "scene.h"
#include "pathtrace.h"
#include "filehandler.h"
#include "main.h"
#include "logging.h"
#include "ui.h"
#include "tile.h"
#include "timer.h"

void computeStatistics(struct renderer *r, int thread, unsigned long long milliseconds, unsigned long long samples);

void printStats(struct renderer *r, unsigned long long ms, unsigned long long samples, int thread) {
#ifdef WINDOWS
	WaitForSingleObject(r->tileMutex, INFINITE);
#else
	pthread_mutex_lock(&r->tileMutex);
#endif
	computeStatistics(r, thread, ms, samples);
#ifdef WINDOWS
	ReleaseMutex(r->tileMutex);
#else
	pthread_mutex_unlock(&r->tileMutex);
#endif
}

/**
 //TODO: Refactor this to retrieve pixel from a given buffer, so we can reuse it for texture maps
 Gets a pixel from the render buffer
 
 @param x X coordinate of pixel
 @param y Y coordinate of pixel
 @return A color object, with full color precision intact (double)
 */
struct color getPixel(struct renderer *r, int x, int y) {
	struct color output = {0.0, 0.0, 0.0, 0.0};
	output.red = r->renderBuffer[(x + (r->image->size.height - y) * r->image->size.width)*3 + 0];
	output.green = r->renderBuffer[(x + (r->image->size.height - y) * r->image->size.width)*3 + 1];
	output.blue = r->renderBuffer[(x + (r->image->size.height - y) * r->image->size.width)*3 + 2];
	output.alpha = 1.0;
	return output;
}

void smartTime(unsigned long long milliseconds, char *buf) {
	time_t secs = milliseconds / 1000;
	time_t mins = secs / 60;
	time_t hours = (secs / 60) / 60;
	
	char secstring[25];
	unsigned long long remainderSeconds = secs - (mins * 60);
	if (remainderSeconds < 10) {
		sprintf(secstring, "0%llu", remainderSeconds);
	} else {
		sprintf(secstring, "%llu", remainderSeconds);
	}
	
	if (mins > 60) {
		sprintf(buf, "%lih %lim", hours, mins - (hours * 60));
	} else if (secs > 60) {
		sprintf(buf, "%lim %ss", mins, secstring);
	} else if (secs > 0) {
		sprintf(buf, "%.2fs", (float)milliseconds / 1000);
	} else {
		sprintf(buf, "%llums", milliseconds);
	}
}

/**
 Print running average duration of tiles rendered

 @param avgTime Current computed average time
 @param remainingTileCount Tiles remaining to render, to compute estimated remaining render time.
 */
void printStatistics(struct renderer *r, int thread, float kSamplesPerSecond) {
	int remainingTileCount = r->tileCount - r->finishedTileCount;
	unsigned long long remainingTimeMilliseconds = (remainingTileCount * r->avgTileTime) / r->threadCount;
	//First print avg tile time
	printf("%s", "\33[2K");
	float completion = ((float)r->finishedTileCount / r->tileCount) * 100;
	logr(info, "[%.0f%%]", completion);
	
	char avg[32];
	smartTime(r->avgTileTime, avg);
	printf(" avgt: %s", avg);
	char rem[32];
	smartTime(remainingTimeMilliseconds, rem);
	printf(", etf: %s, %.2fkS/s%s", rem, kSamplesPerSecond, "\r");
}

void computeStatistics(struct renderer *r, int thread, unsigned long long milliseconds, unsigned long long samples) {
	r->avgTileTime = r->avgTileTime * (r->timeSampleCount - 1);
	r->avgTileTime += milliseconds;
	r->avgTileTime /= r->timeSampleCount;
	
	float multiplier = (float)milliseconds / (float)1000.0f;
	float samplesPerSecond = (float)samples / multiplier;
	samplesPerSecond *= r->threadCount;
	r->avgSampleRate = r->avgSampleRate * (r->timeSampleCount - 1);
	r->avgSampleRate += samplesPerSecond;
	r->avgSampleRate /= (float)r->timeSampleCount;
	
	float printable = (float)r->avgSampleRate / 1000.0f;
	
	printStatistics(r, thread, printable);
	r->timeSampleCount++;
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
		//Return possible codes here
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
	renderer->scene->lights = (struct light*)calloc(1, sizeof(struct light));
	
#ifdef UI_ENABLED
	renderer->mainDisplay = (struct display*)calloc(1, sizeof(struct display));
	renderer->mainDisplay->window = NULL;
	renderer->mainDisplay->renderer = NULL;
	renderer->mainDisplay->texture = NULL;
	renderer->mainDisplay->overlayTexture = NULL;
#endif
	
	//Mutex
#ifdef _WIN32
	renderer->tileMutex = CreateMutex(NULL, FALSE, NULL);
#else
	renderer->tileMutex = (pthread_mutex_t)PTHREAD_MUTEX_INITIALIZER;
#endif
	return renderer;
}
