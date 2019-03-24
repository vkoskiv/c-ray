//
//  renderer.h
//  C-ray
//
//  Created by Valtteri Koskivuori on 19/02/2017.
//  Copyright © 2015-2019 Valtteri Koskivuori. All rights reserved.
//

#pragma once

struct world;
struct texture;
struct display;

/**
 Thread information struct to communicate with main thread
 */
struct threadInfo {
#ifdef WINDOWS
	HANDLE thread_handle;
	DWORD thread_id;
#else
	pthread_t thread_id;
#endif
	int thread_num;
	bool threadComplete;
	struct renderer *r;
};

enum renderOrder {
	renderOrderTopToBottom = 0,
	renderOrderFromMiddle,
	renderOrderToMiddle,
	renderOrderNormal,
	renderOrderRandom
};

/**
 Main renderer. Stores needed information to keep track of render status,
 as well as information needed for the rendering routines.
 */
struct renderer {
	//Source data
	struct world *scene; //Scene to render
	
	//State data
	struct texture *image; //Output image
	struct renderTile *renderTiles; //Array of renderTiles to render
	int tileCount; //Total amount of render tiles
	int finishedTileCount; //Completed render tiles
	double *renderBuffer;  //Double-precision buffer for multisampling
	unsigned char *uiBuffer; //UI element buffer
	int activeThreads; //Amount of threads currently rendering
	bool isRendering;
	bool *threadPaused; //SDL listens for P key pressed, which sets these, one for each thread.
	bool renderAborted;//SDL listens for X key pressed, which sets this
	unsigned long long avgTileTime;//Used for render duration estimation (milliseconds)
	float avgSampleRate; //In raw single pixel samples per second. (Used for benchmarking)
	int timeSampleCount;//Used for render duration estimation, amount of time samples captured
	struct threadInfo *renderThreadInfo; //Info about threads
	pcg32_random_t *rngs; // PCG rng, one for each thread
#ifndef WINDOWS
	pthread_attr_t renderThreadAttributes;
#endif
#ifdef UI_ENABLED
	struct display *mainDisplay;
#endif
	
	//Tile duration timers (one for each thread
	struct timeval *timers;
	
#ifdef WINDOWS
	HANDLE tileMutex; // = INVALID_HANDLE_VALUE;
#else
	pthread_mutex_t tileMutex; // = PTHREAD_MUTEX_INITIALIZER;
#endif
	
	//Preferences data (Set by user)
	enum fileMode mode;
	enum renderOrder tileOrder;
	char *inputFilePath; //Directory to load input files from
	
	int threadCount; //Amount of threads to render with
	int sampleCount;
	int tileWidth;
	int tileHeight;
	
	bool antialiasing;
};

//Renderer
#ifdef WINDOWS
DWORD WINAPI renderThread(LPVOID arg);
#else
void *renderThread(void *arg);
#endif

#ifdef WINDOWS
DWORD WINAPI renderThreadSIMD(LPVOID arg);
#else
void *renderThreadSIMD(void *arg);
#endif

//Initialize a new renderer
struct renderer *newRenderer(void);

//Start main render loop
void render(struct renderer *r);

void freeRenderer(struct renderer *r);
