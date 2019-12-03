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
	
	long avgRayTime;
	
	struct renderer *r;
};

enum renderOrder {
	renderOrderTopToBottom = 0,
	renderOrderFromMiddle,
	renderOrderToMiddle,
	renderOrderNormal,
	renderOrderRandom
};

/// Renderer state data
struct state {
	struct texture *image; //Output image
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
	
#ifdef WINDOWS
	HANDLE tileMutex; // = INVALID_HANDLE_VALUE;
#else
	pthread_attr_t renderThreadAttributes;
	pthread_mutex_t tileMutex; // = PTHREAD_MUTEX_INITIALIZER;
#endif
};

/// Preferences data (Set by user)
struct prefs {
	enum fileMode fileMode;
	enum renderOrder tileOrder;
	
	int threadCount; //Amount of threads to render with
	int sampleCount;
	int bounces;
	int tileWidth;
	int tileHeight;
	
	bool antialiasing;
};

/**
 Main renderer. Stores needed information to keep track of render status,
 as well as information needed for the rendering routines.
 */
struct renderer {
	struct world *scene; //Scene to render
	struct state state;  //Internal state
	struct prefs prefs;  //User prefs
	
	//TODO: Consider moving this out of renderer.
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
void render(struct renderer *r);

//Free renderer allocations
void freeRenderer(struct renderer *r);
