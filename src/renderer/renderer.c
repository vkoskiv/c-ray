//
//  renderer.c
//  C-ray
//
//  Created by Valtteri Koskivuori on 19/02/2017.
//  Copyright © 2015-2020 Valtteri Koskivuori. All rights reserved.
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

//Main thread loop speeds
#define paused_msec 100
#define active_msec  16

void *renderThread(void *arg);

/// @todo Use defaultSettings state struct for this.
/// @todo Clean this up, it's ugly.
struct texture *renderFrame(struct renderer *r) {
	struct texture *output = newTexture(char_p, r->prefs.imageWidth, r->prefs.imageHeight, 3);
	
	logr(info, "Starting C-ray renderer for frame %i\n", r->prefs.imgCount);
	
	logr(info, "Rendering at %s%i%s x %s%i%s\n", KWHT, r->prefs.imageWidth, KNRM, KWHT, r->prefs.imageHeight, KNRM);
	logr(info, "Rendering %s%i%s samples with %s%i%s bounces.\n", KBLU, r->prefs.sampleCount, KNRM, KGRN, r->prefs.bounces, KNRM);
	logr(info, "Rendering with %s%d%s%s thread%s",
		 KRED,
		 r->prefs.fromSystem ? r->prefs.threadCount - 2 : r->prefs.threadCount,
		 r->prefs.fromSystem ? "+2" : "",
		 KNRM,
		 r->prefs.threadCount > 1 ? "s.\n" : ".\n");
	
	logr(info, "Pathtracing...\n");
	
	r->state.isRendering = true;
	r->state.renderAborted = false;
	r->state.saveImage = true; // Set to false if user presses X
	
	//Main loop (input)
	float avgSampleTime = 0.0f;
	float finalAvg = 0.0f;
	int pauser = 0;
	int ctr = 1;
	
	//Create render threads (Nonblocking)
	for (int t = 0; t < r->prefs.threadCount; ++t) {
		r->state.threads[t] = (struct crThread){.thread_num = t, .threadComplete = false, .renderer = r, .output = output, .threadFunc = renderThread};
		if (startThread(&r->state.threads[t])) {
			logr(error, "Failed to create a crThread.\n");
		} else {
			r->state.activeThreads++;
		}
	}
	
	//Start main thread loop to handle SDL and statistics computation
	while (r->state.isRendering) {
		getKeyboardInput(r);
		
		if (!r->state.threads[0].paused) {
			drawWindow(r, output);
			for (int t = 0; t < r->prefs.threadCount; ++t) {
				avgSampleTime += r->state.threads[t].avgSampleTime;
			}
			finalAvg += avgSampleTime / r->prefs.threadCount;
			finalAvg /= ctr++;
			sleepMSec(active_msec);
		} else {
			sleepMSec(paused_msec);
		}
		
		//Run the sample printing about 4x/s
		if (pauser == 280 / active_msec) {
			float timePerSingleTileSample = finalAvg;
			uint64_t totalTileSamples = r->state.tileCount * r->prefs.sampleCount;
			uint64_t completedSamples = 0;
			for (int t = 0; t < r->prefs.threadCount; ++t) {
				completedSamples += r->state.threads[t].totalSamples;
			}
			uint64_t remainingTileSamples = totalTileSamples - completedSamples;
			uint64_t msecTillFinished = 0.001f * (timePerSingleTileSample * remainingTileSamples);
			float usPerRay = finalAvg / (r->prefs.tileHeight * r->prefs.tileWidth);
			float sps = (1000000.0f/usPerRay) * r->prefs.threadCount;
			char rem[64];
			smartTime((msecTillFinished) / r->prefs.threadCount, rem);
			logr(info, "[%s%.0f%%%s] μs/path: %.02f, etf: %s, %.02lfMs/s %s        \r",
				 KBLU,
				 ((float)r->state.finishedTileCount / (float)r->state.tileCount) * 100.0f,
				 KNRM,
				 usPerRay,
				 rem,
				 0.000001f * sps,
				 r->state.threads[0].paused ? "[PAUSED]" : "");
			pauser = 0;
		}
		pauser++;
		
		//Wait for render threads to finish (Render finished)
		for (int t = 0; t < r->prefs.threadCount; ++t) {
			if (r->state.threads[t].threadComplete && r->state.threads[t].thread_num != -1) {
				--r->state.activeThreads;
				r->state.threads[t].thread_num = -1; //Mark as checked
			}
			if (!r->state.activeThreads || r->state.renderAborted) {
				r->state.isRendering = false;
			}
		}
	}
	
	//Make sure render threads are terminated before continuing (This blocks)
	for (int t = 0; t < r->prefs.threadCount; ++t) {
		checkThread(&r->state.threads[t]);
	}
	return output;
}

/**
 A render thread
 
 @param arg Thread information (see threadInfo struct)
 @return Exits when thread is done
 */
void *renderThread(void *arg) {
	struct lightRay incidentRay;
	struct crThread *thread = (struct crThread*)arg;
	struct renderer *r = thread->renderer;
	struct texture *image = thread->output;
	sampler *sampler = newSampler();
	
	//First time setup for each thread
	struct renderTile tile = nextTile(r);
	thread->currentTileNum = tile.tileNum;
	
	struct timeval timer = {0};
	
	float aperture = r->scene->camera->aperture;
	float focalDistance = r->scene->camera->focalDistance;
	
	while (tile.tileNum != -1 && r->state.isRendering) {
		long totalUsec = 0;
		long samples = 0;
		
		while (tile.completedSamples < r->prefs.sampleCount+1 && r->state.isRendering) {
			startTimer(&timer);
			for (int y = tile.end.y - 1; y > tile.begin.y - 1; --y) {
				for (int x = tile.begin.x; x < tile.end.x; ++x) {
					if (r->state.renderAborted) return 0;
					uint32_t pixIdx = y * image->width + x;
					initSampler(sampler, Halton, tile.completedSamples - 1, r->prefs.sampleCount, pixIdx);
					
					float fracX = (float)x;
					float fracY = (float)y;
					
					//A cheap 'antialiasing' of sorts. The more samples, the better this works
					float jitter = 0.25f;
					if (r->prefs.antialiasing) {
						fracX = rndFloatRange(fracX - jitter, fracX + jitter, sampler);
						fracY = rndFloatRange(fracY - jitter, fracY + jitter, sampler);
					}
					
					//Set up the light ray to be casted. direction is pointing towards the X,Y coordinate on the
					//imaginary plane in front of the origin. startPos is just the camera position.
					struct vector direction = vecNormalize((struct vector){
												(fracX - 0.5f * image->width) / r->scene->camera->focalLength,
												(fracY - 0.5f * image->height) / r->scene->camera->focalLength,
												1.0f
											});
					struct vector startPos = r->scene->camera->pos;
					struct vector left = r->scene->camera->left;
					struct vector up = r->scene->camera->up;
					
					//Run camera tranforms on direction vector
					transformCameraView(r->scene->camera, &direction);
					
					incidentRay.start = startPos;
					incidentRay.direction = direction;
					incidentRay.rayType = rayTypeIncident;
					
					//Now handle aperture
					if (aperture <= 0.0f) {
						incidentRay.start = startPos;
					} else {
						float ft = focalDistance / direction.z;
						struct vector focusPoint = alongRay(incidentRay, ft);
						
						struct coord lensPoint = coordScale(aperture, randomCoordOnUnitDisc(sampler));
						incidentRay.start = vecAdd(vecAdd(startPos, vecScale(up, lensPoint.y)), vecScale(left, lensPoint.x));
						incidentRay.direction = vecNormalize(vecSub(focusPoint, incidentRay.start));
						
					}
					
					//For multi-sample rendering, we keep a running average of color values for each pixel
					//The next block of code does this
					
					//Get previous color value from render buffer
					struct color output = textureGetPixel(r->state.renderBuffer, x, y);
					
					//Get new sample (path tracing is initiated here)
					//struct color sample = pathTracePreview(&incidentRay, r->scene, 0, r->prefs.bounces, &rng);
					struct color sample = pathTrace(&incidentRay, r->scene, 0, r->prefs.bounces, sampler);
					
					//And process the running average
					output = colorCoef((float)(tile.completedSamples - 1), output);
					
					output = addColors(output, sample);
					
					float t = 1.0f / tile.completedSamples;
					output.red = t * output.red;
					output.green = t * output.green;
					output.blue = t * output.blue;
					
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
			tile.completedSamples++;
			thread->totalSamples++;
			thread->completedSamples = tile.completedSamples;
			//Pause rendering when bool is set
			while (thread->paused && !r->state.renderAborted) {
				sleepMSec(100);
			}
			thread->avgSampleTime = totalUsec / samples;
		}
		//Tile has finished rendering, get a new one and start rendering it.
		r->state.renderTiles[tile.tileNum].isRendering = false;
		r->state.renderTiles[tile.tileNum].renderComplete = true;
		thread->currentTileNum = -1;
		thread->completedSamples = 0;
		tile = nextTile(r);
		thread->currentTileNum = tile.tileNum;
	}
	destroySampler(sampler);
	//No more tiles to render, exit thread. (render done)
	thread->threadComplete = true;
	thread->currentTileNum = -1;
	return 0;
}

struct renderer *newRenderer() {
	struct renderer *r = calloc(1, sizeof(*r));
	r->state.avgTileTime = (time_t)1;
	r->state.timeSampleCount = 1;
	
	r->state.timer = calloc(1, sizeof(*r->state.timer));
	
	//TODO: Do we need all these heap allocs?
	r->scene = calloc(1, sizeof(*r->scene));
	r->scene->camera = calloc(1, sizeof(*r->scene->camera));
	r->scene->hdr = NULL; //Optional, to be loaded later
	r->scene->meshes = calloc(1, sizeof(*r->scene->meshes));
	r->scene->spheres = calloc(1, sizeof(*r->scene->spheres));
	
	if (!vertexArray) {
		allocVertexBuffer();
	}
	
	//Mutex
	r->state.tileMutex = createMutex();
	return r;
}
	
void destroyRenderer(struct renderer *r) {
	if (r->state.timer) {
		free(r->state.timer);
	}
	destroyScene(r->scene);
	if (r->state.renderTiles) {
		free(r->state.renderTiles);
	}
	
	destroyTexture(r->state.renderBuffer);
	destroyTexture(r->state.uiBuffer);
	
	if (r->state.threads) {
		free(r->state.threads);
	}
	if (r->prefs.imgFileName) {
		free(r->prefs.imgFileName);
	}
	if (r->prefs.imgFilePath) {
		free(r->prefs.imgFilePath);
	}
	
	if (vertexArray) {
		destroyVertexBuffer();
	}
	
	free(r);
}
