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

void computeStatistics(struct renderer *r, int thread, unsigned long long milliseconds, unsigned long long samples);
void reorderTiles(struct renderer *renderer);

#ifdef WINDOWS
typedef struct timeval {
	long tv_sec;
	long tv_usec;
} TIMEVAL, *PTIMEVAL, *LPTIMEVAL;

int gettimeofday(struct timeval * tp, struct timezone * tzp) {
	// Note: some broken versions only have 8 trailing zero's, the correct epoch has 9 trailing zero's
	// This magic number is the number of 100 nanosecond intervals since January 1, 1601 (UTC)
	// until 00:00:00 January 1, 1970
	static const uint64_t EPOCH = ((uint64_t) 116444736000000000ULL);
	
	SYSTEMTIME  system_time;
	FILETIME    file_time;
	uint64_t    time;
	
	GetSystemTime( &system_time );
	SystemTimeToFileTime( &system_time, &file_time );
	time =  ((uint64_t)file_time.dwLowDateTime )      ;
	time += ((uint64_t)file_time.dwHighDateTime) << 32;
	
	tp->tv_sec  = (long) ((time - EPOCH) / 10000000L);
	tp->tv_usec = (long) (system_time.wMilliseconds * 1000);
	return 0;
}
#endif

//Timer funcs
void startTimer(struct timeval *timer) {
	gettimeofday(timer, NULL);
}

unsigned long long endTimer(struct timeval *timer) {
	struct timeval tmr2;
	gettimeofday(&tmr2, NULL);
	return 1000 * (tmr2.tv_sec - timer->tv_sec) + ((tmr2.tv_usec - timer->tv_usec) / 1000);
}

/**
 Gets the next tile from renderTiles in mainRenderer
 
 @return A renderTile to be rendered
 */
struct renderTile getTile(struct renderer *r) {
	struct renderTile tile;
	memset(&tile, 0, sizeof(tile));
	tile.tileNum = -1;
#ifdef WINDOWS
	WaitForSingleObject(r->tileMutex, INFINITE);
#else
	pthread_mutex_lock(&r->tileMutex);
#endif
	if (r->finishedTileCount < r->tileCount) {
		tile = r->renderTiles[r->finishedTileCount];
		r->renderTiles[r->finishedTileCount].isRendering = true;
		tile.tileNum = r->finishedTileCount++;
	}
#ifdef WINDOWS
	ReleaseMutex(r->tileMutex);
#else
	pthread_mutex_unlock(&r->tileMutex);
#endif
	return tile;
}

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
 Create tiles from render plane, and add those to mainRenderer
 
 @param scene scene object
 */
void quantizeImage(struct renderer *r) {
	
	logr(info, "Quantizing render plane...\n");
	
	//Sanity check on tilesizes
	if (r->tileWidth >= r->image->size.width) r->tileWidth = r->image->size.width;
	if (r->tileHeight >= r->image->size.height) r->tileHeight = r->image->size.height;
	if (r->tileWidth <= 0) r->tileWidth = 1;
	if (r->tileHeight <= 0) r->tileHeight = 1;
	
	int tilesX = r->image->size.width / r->tileWidth;
	int tilesY = r->image->size.height / r->tileHeight;
	
	tilesX = (r->image->size.width % r->tileWidth) != 0 ? tilesX + 1: tilesX;
	tilesY = (r->image->size.height % r->tileHeight) != 0 ? tilesY + 1: tilesY;
	
	r->renderTiles = (struct renderTile*)calloc(tilesX*tilesY, sizeof(struct renderTile));
	if (r->renderTiles == NULL) {
		logr(error, "Failed to allocate renderTiles array.\n");
	}
	
	for (int y = 0; y < tilesY; y++) {
		for (int x = 0; x < tilesX; x++) {
			struct renderTile *tile = &r->renderTiles[x + y*tilesX];
			tile->width  = r->tileWidth;
			tile->height = r->tileHeight;
			
			tile->begin.x = x       * r->tileWidth;
			tile->end.x   = (x + 1) * r->tileWidth;
			
			tile->begin.y = y       * r->tileHeight;
			tile->end.y   = (y + 1) * r->tileHeight;
			
			tile->end.x = min((x + 1) * r->tileWidth, r->image->size.width);
			tile->end.y = min((y + 1) * r->tileHeight, r->image->size.height);
			
			//Samples have to start at 1, so the running average works
			tile->completedSamples = 1;
			tile->isRendering = false;
			tile->tileNum = r->tileCount;
			
			r->tileCount++;
		}
	}
	logr(info, "Quantized image into %i tiles. (%ix%i)\n", (tilesX*tilesY), tilesX, tilesY);
	
	reorderTiles(r);
}

/**
 Reorder renderTiles to start from top
 */
void reorderTopToBottom(struct renderer *r) {
	int endIndex = r->tileCount - 1;
	
	struct renderTile *tempArray = (struct renderTile*)calloc(r->tileCount, sizeof(struct renderTile));
	
	for (int i = 0; i < r->tileCount; i++) {
		tempArray[i] = r->renderTiles[endIndex--];
	}
	
	free(r->renderTiles);
	r->renderTiles = tempArray;
}

unsigned int rand_interval(unsigned int min, unsigned int max) {
	unsigned int r;
	const unsigned int range = 1 + max - min;
	const unsigned int buckets = RAND_MAX / range;
	const unsigned int limit = buckets * range;
	
	/* Create equal size buckets all in a row, then fire randomly towards
	 * the buckets until you land in one of them. All buckets are equally
	 * likely. If you land off the end of the line of buckets, try again. */
	do
	{
		r = rand();
	} while (r >= limit);
	
	return min + (r / buckets);
}

/**
 Shuffle renderTiles into a random order
 */
void reorderRandom(struct renderer *r) {
	for (int i = 0; i < r->tileCount; i++) {
		unsigned int random = rand_interval(0, r->tileCount - 1);
		
		struct renderTile temp = r->renderTiles[i];
		r->renderTiles[i] = r->renderTiles[random];
		r->renderTiles[random] = temp;
	}
}

/**
 Reorder renderTiles to start from middle
 */
void reorderFromMiddle(struct renderer *r) {
	int midLeft = 0;
	int midRight = 0;
	bool isRight = true;
	
	midRight = ceil(r->tileCount / 2);
	midLeft = midRight - 1;
	
	struct renderTile *tempArray = (struct renderTile*)calloc(r->tileCount, sizeof(struct renderTile));
	
	for (int i = 0; i < r->tileCount; i++) {
		if (isRight) {
			tempArray[i] = r->renderTiles[midRight++];
			isRight = false;
		} else {
			tempArray[i] = r->renderTiles[midLeft--];
			isRight = true;
		}
	}
	
	free(r->renderTiles);
	r->renderTiles = tempArray;
}


/**
 Reorder renderTiles to start from ends, towards the middle
 */
void reorderToMiddle(struct renderer *r) {
	int left = 0;
	int right = 0;
	bool isRight = true;
	
	right = r->tileCount - 1;
	
	struct renderTile *tempArray = (struct renderTile*)calloc(r->tileCount, sizeof(struct renderTile));
	
	for (int i = 0; i < r->tileCount; i++) {
		if (isRight) {
			tempArray[i] = r->renderTiles[right--];
			isRight = false;
		} else {
			tempArray[i] = r->renderTiles[left++];
			isRight = true;
		}
	}
	
	free(r->renderTiles);
	r->renderTiles = tempArray;
}

/**
 Reorder renderTiles in given order
 
 @param order Render order to be applied
 */
void reorderTiles(struct renderer *r) {
	switch (r->tileOrder) {
		case renderOrderFromMiddle:
		{
			reorderFromMiddle(r);
		}
			break;
		case renderOrderToMiddle:
		{
			reorderToMiddle(r);
		}
			break;
		case renderOrderTopToBottom:
		{
			reorderTopToBottom(r);
		}
			break;
		case renderOrderRandom:
		{
			reorderRandom(r);
		}
			break;
		default:
			break;
	}
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

/**
 Compute the running average time from a given tile's render duration

 @param tile Tile to get the duration from
 */
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
		
		while (tile.tileNum != -1) {
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
				while (renderer->threadPaused[tinfo->thread_num]) {
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
	
void initRenderer(struct renderer *renderer) {
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
	
	//Alloc timers
	renderer->timers = (struct timeval*)calloc(renderer->threadCount, sizeof(struct timeval));
	//Mutex
#ifdef _WIN32
	renderer->tileMutex = CreateMutex(NULL, FALSE, NULL);
#else
	renderer->tileMutex = (pthread_mutex_t)PTHREAD_MUTEX_INITIALIZER;
#endif
}
