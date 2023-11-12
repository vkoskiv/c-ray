//
//  tile.h
//  C-ray
//
//  Created by Valtteri Koskivuori on 06/07/2018.
//  Copyright © 2018-2022 Valtteri Koskivuori. All rights reserved.
//

#pragma once

#include "../includes.h"
#include "../utils/dyn_array.h"

#include "vector.h"

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

typedef struct render_tile render_tile;
dyn_array_def(render_tile);

/// Quantize the render plane into an array of tiles, with properties as specified in the parameters below
unsigned tile_quantize(struct render_tile_arr *tiles, unsigned width, unsigned height, unsigned tileWidth, unsigned tileHeight, enum render_order tileOrder);

struct render_tile *tile_next(struct renderer *r);

struct render_tile *tile_next_interactive(struct renderer *r);
