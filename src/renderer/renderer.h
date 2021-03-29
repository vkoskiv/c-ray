//
//  renderer.h
//  C-ray
//
//  Created by Valtteri Koskivuori on 19/02/2017.
//  Copyright © 2017-2020 Valtteri Koskivuori. All rights reserved.
//

#pragma once

enum renderOrder {
	renderOrderTopToBottom = 0,
	renderOrderFromMiddle,
	renderOrderToMiddle,
	renderOrderNormal,
	renderOrderRandom
};

struct renderThreadState {
	int thread_num;
	bool threadComplete;
	
	bool paused; //SDL listens for P key pressed, which sets these, one for each thread.
	
	//Share info about the current tile with main thread
	int currentTileNum;
	int completedSamples;
	
	uint64_t totalSamples;
	
	long avgSampleTime; //Single tile pass
	
	struct renderer *renderer;
	struct texture *output;
};

/// Renderer state data
struct state {
	struct renderTile *renderTiles; //Array of renderTiles to render
	int tileCount; //Total amount of render tiles
	int finishedTileCount;
	int finishedPasses; // For interactive mode
	struct texture *renderBuffer; //float-precision buffer for multisampling
	struct texture *uiBuffer; //UI element buffer
	int activeThreads; //Amount of threads currently rendering
	bool isRendering;
	bool renderAborted; //SDL listens for X key pressed, which sets this
	bool saveImage;
	unsigned long long avgTileTime; //Used for render duration estimation (milliseconds)
	float avgSampleRate; //In raw single pixel samples per second. (Used for benchmarking)
	int timeSampleCount; //Used for render duration estimation, amount of time samples captured
	struct crThread *threads; //Render threads
	struct renderThreadState *threadStates;
	struct renderClient *clients;
	size_t clientCount;
	struct timeval *timer;
	
	struct crMutex *tileMutex;
};

/// Preferences data (Set by user)
struct prefs {
	enum renderOrder tileOrder;
	
	int threadCount; //Amount of threads to render with
	bool fromSystem; //Did we ask the system for thread count
	int sampleCount;
	int bounces;
	unsigned tileWidth;
	unsigned tileHeight;
	
	//Output prefs
	unsigned imageWidth;
	unsigned imageHeight;
	char *imgFilePath;
	char *imgFileName;
	char *assetPath;
	int imgCount;
	enum fileType imgType;
	
	//Preview window prefs
	//These are passed to initDisplay(), shouldn't have these here.
	bool enabled;
	bool fullscreen;
	bool borderless;
	bool useClustering;
	bool isWorker;
	float scale;
	
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
	char *sceneCache;    //Packed scene data that can be passed to workers
};

//Initialize a new renderer
struct renderer *newRenderer(void);

//Start main render loop
struct texture *renderFrame(struct renderer *r);

//Free renderer allocations
void destroyRenderer(struct renderer *r);
