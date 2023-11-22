//
//  renderer.h
//  C-ray
//
//  Created by Valtteri Koskivuori on 19/02/2017.
//  Copyright © 2017-2022 Valtteri Koskivuori. All rights reserved.
//

#pragma once

#include <c-ray/c-ray.h>
#include "../datatypes/tile.h"
#include "../datatypes/image/imagefile.h"
#include "../utils/timer.h"
#include "../utils/filecache.h"
#include "../utils/platform/thread.h"
#include "../utils/protocol/server.h"

struct worker {
	struct cr_thread thread;
	bool thread_complete;
	
	bool paused; //SDL listens for P key pressed, which sets these, one for each thread.
	
	//Share info about the current tile with main thread
	struct render_tile *currentTile;
	size_t completedSamples; //FIXME: Remove
	uint64_t totalSamples;
	
	long avg_per_sample_us; //Single tile pass

	struct camera *cam;
	struct renderer *renderer;
	struct texture *output;
	struct render_client *client; // Optional
};
typedef struct worker worker;
dyn_array_def(worker);

/// Renderer state data
struct state {
	struct render_tile_arr tiles;
	struct cr_mutex *tile_mutex;

	size_t finishedTileCount;
	size_t finishedPasses; // For interactive mode
	struct texture *renderBuffer; //float-precision buffer for multisampling
	bool rendering;
	bool render_aborted; //SDL listens for X key pressed, which sets this
	bool saveImage;
	struct worker_arr workers;
	struct render_client_arr clients;
	struct file_cache *file_cache; // A file cache for network render nodes. NULL if only local render.
	
	struct cr_renderer_callbacks cb;
};

/// Preferences data (Set by user)
struct prefs {
	enum render_order tileOrder;
	
	size_t threads; //Amount of threads to render with
	bool fromSystem; //Did we ask the system for thread count
	size_t sampleCount;
	size_t bounces;
	unsigned tileWidth;
	unsigned tileHeight;
	
	//Output prefs
	unsigned override_width;
	unsigned override_height;
	size_t selected_camera;
	char *imgFilePath;
	char *imgFileName;
	char *assetPath;
	size_t imgCount;
	enum fileType imgType;
	char *node_list;
	bool iterative;
};

struct renderer {
	struct world *scene; //Scene to render
	struct state state;  //Internal state
	struct prefs prefs;  //User prefs
	char *sceneCache;    //Packed scene data that can be passed to workers
};

//Initialize a new renderer
struct renderer *renderer_new(void);

//Start main render loop
struct texture *renderFrame(struct renderer *r);

//Free renderer allocations
void renderer_destroy(struct renderer *r);
