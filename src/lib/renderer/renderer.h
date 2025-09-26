//
//  renderer.h
//  c-ray
//
//  Created by Valtteri Koskivuori on 19/02/2017.
//  Copyright © 2017-2025 Valtteri Koskivuori. All rights reserved.
//

#pragma once

#include <v.h>
#include <c-ray/c-ray.h>
#include <datatypes/tile.h>
#include <protocol/server.h>

struct worker {
	v_thread_ctx thread_ctx;
	v_thread *thread;
	bool thread_complete;
	
	bool paused; //SDL listens for P key pressed, which sets these, one for each thread.
	bool in_pause_loop;
	
	//Share info about the current tile with main thread
	struct tile_set *tiles;
	struct render_tile *currentTile;
	uint64_t totalSamples;
	
	long avg_per_sample_us; //Single tile pass

	struct camera *cam;
	struct renderer *renderer;
	struct texture **buf;
	struct render_client *client; // Optional
};
typedef struct worker worker;
v_arr_def(worker)

struct callback {
	void (*fn)(struct cr_renderer_cb_info *, void *);
	void *user_data;
};

enum renderer_state {
	r_idle = 0,
	r_rendering,
	r_restarting,
	r_exiting,
};

/// Renderer state data
struct state {
	size_t finishedPasses; // For interactive mode
	enum renderer_state s;
	struct worker_arr workers;
	struct render_client_arr clients;
	// TODO: Single callback that has event type as first arg
	// instead of this awkward set of several different callbacks
	// for different events
	struct callback callbacks[5];

	struct texture *result_buf;
	struct tile_set *current_set;
};

/// Preferences data (Set by user)
struct prefs {
	enum render_order tileOrder;
	
	size_t threads; //Amount of threads to render with
	size_t sampleCount;
	size_t bounces;
	unsigned tileWidth;
	unsigned tileHeight;
	
	//Output prefs
	unsigned override_width;
	unsigned override_height;
	size_t selected_camera;
	char *node_list;
	bool iterative;
	bool blender_mode;
};

struct renderer {
	struct world *scene; //Scene to render
	struct state state;  //Internal state
	struct prefs prefs;  //User prefs
};

struct renderer *renderer_new(void);
void renderer_render(struct renderer *r);
void renderer_start_interactive(struct renderer *r);
void renderer_destroy(struct renderer *r);

// Exposed for now, so API calls can synchronously ensure the BVH is up to date
void update_toplevel_bvh(struct world *s);

struct prefs default_prefs(void); // TODO: Remove
