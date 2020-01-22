//
//  renderer.h
//  C-ray
//
//  Created by Valtteri Koskivuori on 19/02/2017.
//  Copyright © 2015-2020 Valtteri Koskivuori. All rights reserved.
//

#pragma once
/**
 Thread information struct to communicate with main thread
 */
struct threadState {
#ifdef WINDOWS
	HANDLE thread_handle;
	DWORD thread_id;
#else
	pthread_t thread_id;
#endif
	int thread_num;
	bool threadComplete;
	
	bool paused; //SDL listens for P key pressed, which sets these, one for each thread.
	
	//Share info about the current tile with main thread
	int currentTileNum;
	int completedSamples;
	
	uint64_t totalSamples;
	
	long avgSampleTime; //Single tile pass
	
	struct renderer *r;
	struct texture *output;
};

struct timeval;

/// Renderer state data
struct state {
	struct renderTile *renderTiles; //Array of renderTiles to render
	int tileCount; //Total amount of render tiles
	int finishedTileCount;
	struct texture *renderBuffer;  //float-precision buffer for multisampling
	struct texture *uiBuffer; //UI element buffer
	int activeThreads; //Amount of threads currently rendering
	bool isRendering;
	bool renderAborted;//SDL listens for X key pressed, which sets this
	unsigned long long avgTileTime;//Used for render duration estimation (milliseconds)
	float avgSampleRate; //In raw single pixel samples per second. (Used for benchmarking)
	int timeSampleCount;//Used for render duration estimation, amount of time samples captured
	struct threadState *threadStates; //Info about threads
	struct timeval *timer;
#ifdef WINDOWS
	HANDLE tileMutex; // = INVALID_HANDLE_VALUE;
#else
	pthread_attr_t renderThreadAttributes;
	pthread_mutex_t tileMutex; // = PTHREAD_MUTEX_INITIALIZER;
#endif
};

/// Preferences data (Set by user)
struct prefs {
	enum renderOrder tileOrder;
	
	int threadCount; //Amount of threads to render with
	bool fromSystem; //Did we ask the system for thread count
	int sampleCount;
	int bounces;
	int tileWidth;
	int tileHeight;
	
	//Output prefs
	unsigned imageWidth;
	unsigned imageHeight;
	char *imgFilePath;
	char *imgFileName;
	int imgCount;
	enum fileType imgType;
	
	bool antialiasing;
};

/**
 Main renderer. Stores needed information to keep track of render status,
 as well as information needed for the rendering routines.
 */
//FIXME: Completely hide state, scene and mainDisplay as opaque pointers and expose via API
struct renderer {
	struct world *scene; //Scene to render
	struct state state;  //Internal state
	struct prefs prefs;  //User prefs
	
	//TODO: Consider moving this out of renderer.
	//FIXME: rename to just display
	struct display *mainDisplay;
};

//Renderer
#ifdef WINDOWS
DWORD WINAPI renderThread(LPVOID arg);
#else
void *renderThread(void *arg);
#endif

//Initialize a new renderer
struct renderer *newRenderer(void);

//Start main render loop
struct texture *renderFrame(struct renderer *r);

//Free renderer allocations
void freeRenderer(struct renderer *r);
