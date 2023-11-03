//
//  renderer.h
//  C-ray
//
//  Created by Valtteri Koskivuori on 19/02/2017.
//  Copyright © 2017-2022 Valtteri Koskivuori. All rights reserved.
//

#pragma once

#include "../datatypes/tile.h"
#include "../datatypes/image/imagefile.h"
#include "../utils/timer.h"
#include "../utils/filecache.h"
#include "../utils/platform/thread.h"

struct worker {
	struct cr_thread thread;
	bool thread_complete;
	
	bool paused; //SDL listens for P key pressed, which sets these, one for each thread.
	
	//Share info about the current tile with main thread
	struct renderTile *currentTile;
	size_t completedSamples; //FIXME: Remove
	uint64_t totalSamples;
	
	long avgSampleTime; //Single tile pass

	struct camera *cam;
	struct renderer *renderer;
	struct texture *output;
	struct renderClient *client; // Optional
};

/// Renderer state data
struct state {
	struct renderTile *renderTiles; //Array of renderTiles to render
	size_t tileCount; //Total amount of render tiles
	size_t finishedTileCount;
	size_t finishedPasses; // For interactive mode
	struct texture *renderBuffer; //float-precision buffer for multisampling
	size_t activeThreads; //Amount of threads currently rendering
	bool rendering;
	bool render_aborted; //SDL listens for X key pressed, which sets this
	bool saveImage;
	unsigned long long avgTileTime; //Used for render duration estimation (milliseconds)
	float avgSampleRate; //In raw single pixel samples per second. (Used for benchmarking)
	size_t timeSampleCount; //Used for render duration estimation, amount of time samples captured
	struct worker *workers;
	struct renderClient *clients;
	size_t clientCount;
	struct timeval timer;
	struct file_cache *file_cache; // A file cache for network render nodes. NULL if only local render.
	
	struct cr_mutex *tileMutex;
};

struct sdl_prefs {
	bool enabled;
	bool fullscreen;
	bool borderless;
	float scale;
};

/// Preferences data (Set by user)
struct prefs {
	enum renderOrder tileOrder;
	
	size_t threads; //Amount of threads to render with
	bool fromSystem; //Did we ask the system for thread count
	size_t sampleCount;
	size_t bounces;
	unsigned tileWidth;
	unsigned tileHeight;
	
	//Output prefs
	bool override_dimensions;
	unsigned override_width;
	unsigned override_height;
	size_t selected_camera;
	char *imgFilePath;
	char *imgFileName;
	char *assetPath;
	size_t imgCount;
	enum fileType imgType;
	bool useClustering;
	bool isWorker;
	struct sdl_prefs window;
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
	struct sdl_window *sdl; //FIXME: Temporarily here
};

//Initialize a new renderer
struct renderer *newRenderer(void);

//Start main render loop
struct texture *renderFrame(struct renderer *r);

//Free renderer allocations
void destroyRenderer(struct renderer *r);
