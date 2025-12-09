//
//  tile.h
//  c-ray
//
//  Created by Valtteri Koskivuori on 06/07/2018.
//  Copyright © 2018-2025 Valtteri Koskivuori. All rights reserved.
//

#pragma once

#include <v.h>

#include "../../includes.h"

#include <common/vector.h>

enum render_order {
	ro_top_to_bottom = 0,
	ro_from_middle,
	ro_to_middle,
	ro_normal,
	ro_random
};

struct renderer;

enum tile_state {
	ready_to_render = 0,
	rendering,
	finished
};

struct render_tile {
	unsigned width;
	unsigned height;
	struct intCoord begin;
	struct intCoord end;
	enum tile_state state;
	bool network_renderer; //FIXME: client struct ptr
	int index;
	size_t total_samples;
	size_t completed_samples;
};

struct tile_set {
	struct render_tile *tiles;
	size_t finished;
	struct v_mutex *tile_mutex;
};

struct render_tile *tile_quantize(unsigned width, unsigned height, unsigned tile_w, unsigned tile_h, enum render_order order);
void tile_set_free(struct tile_set *set);

struct render_tile *tile_next(struct tile_set *set);

struct render_tile *tile_next_interactive(struct renderer *r, struct tile_set *set);
