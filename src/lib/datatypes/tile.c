//
//  tile.c
//  C-ray
//
//  Created by Valtteri Koskivuori on 06/07/2018.
//  Copyright © 2018-2022 Valtteri Koskivuori. All rights reserved.
//

#include "../../includes.h"
#include "../renderer/renderer.h"
#include "tile.h"

#include "../../common/logging.h"
#include "../../common/platform/mutex.h"
#include "../../vendored/pcg_basic.h"
#include <string.h>

static void tiles_reorder(struct render_tile_arr *tiles, enum render_order tileOrder);

struct render_tile *tile_next(struct tile_set *set) {
	struct render_tile *tile = NULL;
	mutex_lock(set->tile_mutex);
	if (set->finished < set->tiles.count) {
		tile = &set->tiles.items[set->finished];
		tile->state = rendering;
		tile->index = set->finished++;
	} else {
		// If a network worker disappeared during render, finish those tiles locally here at the end
		for (size_t t = 0; t < set->tiles.count; ++t) {
			if (set->tiles.items[t].state == rendering && set->tiles.items[t].network_renderer) {
				set->tiles.items[t].network_renderer = false;
				tile = &set->tiles.items[t];
				tile->state = rendering;
				tile->index = t;
				break;
			}
		}
	}
	mutex_release(set->tile_mutex);
	return tile;
}

struct render_tile *tile_next_interactive(struct renderer *r, struct tile_set *set) {
	struct render_tile *tile = NULL;
	mutex_lock(set->tile_mutex);
	again:
	if (r->state.finishedPasses < r->prefs.sampleCount + 1) {
		if (set->finished < set->tiles.count) {
			tile = &set->tiles.items[set->finished];
			tile->state = rendering;
			tile->index = set->finished++;
		} else {
			r->state.finishedPasses++;
			set->finished = 0;
			goto again;
		}
	}
	mutex_release(set->tile_mutex);
	return tile;
}

struct tile_set tile_quantize(unsigned width, unsigned height, unsigned tile_w, unsigned tile_h, enum render_order order) {

	logr(info, "Quantizing render plane\n");

	struct tile_set set = { 0 };
	set.tile_mutex = mutex_create();

	//Sanity check on tilesizes
	if (tile_w >= width) tile_w = width;
	if (tile_h >= height) tile_h = height;
	if (tile_w <= 0) tile_w = 1;
	if (tile_h <= 0) tile_h = 1;

	unsigned tiles_x = width / tile_w;
	unsigned tiles_y = height / tile_h;

	tiles_x = (width % tile_w) != 0 ? tiles_x + 1: tiles_x;
	tiles_y = (height % tile_h) != 0 ? tiles_y + 1: tiles_y;

	int tileCount = 0;
	for (unsigned y = 0; y < tiles_y; ++y) {
		for (unsigned x = 0; x < tiles_x; ++x) {
			struct render_tile tile = { 0 };
			tile.width  = tile_w;
			tile.height = tile_h;
			
			tile.begin.x = x       * tile_w;
			tile.end.x   = (x + 1) * tile_w;
			
			tile.begin.y = y       * tile_h;
			tile.end.y   = (y + 1) * tile_h;
			
			tile.end.x = min((x + 1) * tile_w, width);
			tile.end.y = min((y + 1) * tile_h, height);
			
			tile.width = tile.end.x - tile.begin.x;
			tile.height = tile.end.y - tile.begin.y;

			tile.state = ready_to_render;

			tile.index = tileCount++;
			render_tile_arr_add(&set.tiles, tile);
		}
	}
	logr(info, "Quantized image into %i tiles. (%ix%i)\n", (tiles_x * tiles_y), tiles_x, tiles_y);

	tiles_reorder(&set.tiles, order);

	return set;
}

void tile_set_free(struct tile_set *set) {
	render_tile_arr_free(&set->tiles);
	if (set->tile_mutex) free(set->tile_mutex);
	set->tile_mutex = NULL;
}

static void reorder_top_to_bottom(struct render_tile_arr *tiles) {
	struct render_tile_arr temp = { 0 };
	
	for (unsigned i = 0; i < tiles->count; ++i) {
		render_tile_arr_add(&temp, tiles->items[tiles->count - i - 1]);
	}
	
	render_tile_arr_free(tiles);
	*tiles = temp;
}

static unsigned int rand_interval(unsigned int min, unsigned int max, pcg32_random_t *rng) {
	unsigned int r;
	const unsigned int range = 1 + max - min;
	const unsigned int buckets = UINT32_MAX / range;
	const unsigned int limit = buckets * range;
	
	/* Create equal size buckets all in a row, then fire randomly towards
	 * the buckets until you land in one of them. All buckets are equally
	 * likely. If you land off the end of the line of buckets, try again. */
	do {
		r = pcg32_random_r(rng);
	} while (r >= limit);
	
	return min + (r / buckets);
}

static void reorder_random(struct render_tile_arr *tiles) {
	pcg32_random_t rng;
	pcg32_srandom_r(&rng, 3141592, 0);
	for (unsigned i = 0; i < tiles->count; ++i) {
		unsigned random = rand_interval(0, tiles->count - 1, &rng);
		
		struct render_tile temp = tiles->items[i];
		tiles->items[i] = tiles->items[random];
		tiles->items[random] = temp;
	}
}

static void reorder_from_middle(struct render_tile_arr *tiles) {
	int mid_left = 0;
	int mid_right = 0;
	bool is_right = true;
	
	mid_right = ceil(tiles->count / 2);
	mid_left = mid_right - 1;
	
	struct render_tile_arr temp = { 0 };
	
	for (unsigned i = 0; i < tiles->count; ++i) {
		if (is_right) {
			render_tile_arr_add(&temp, tiles->items[mid_right++]);
			is_right = false;
		} else {
			render_tile_arr_add(&temp, tiles->items[mid_left--]);
			is_right = true;
		}
	}
	
	render_tile_arr_free(tiles);
	*tiles = temp;
}

static void reorder_to_middle(struct render_tile_arr *tiles) {
	unsigned left = 0;
	unsigned right = 0;
	bool isRight = true;
	
	right = tiles->count - 1;
	
	struct render_tile_arr temp = { 0 };
	
	for (unsigned i = 0; i < tiles->count; ++i) {
		if (isRight) {
			render_tile_arr_add(&temp, tiles->items[right--]);
			isRight = false;
		} else {
			render_tile_arr_add(&temp, tiles->items[left++]);
			isRight = true;
		}
	}
	
	render_tile_arr_free(tiles);
	*tiles = temp;
}

static void tiles_reorder(struct render_tile_arr *tiles, enum render_order tileOrder) {
	switch (tileOrder) {
		case ro_from_middle:
			reorder_from_middle(tiles);
			break;
		case ro_to_middle:
			reorder_to_middle(tiles);
			break;
		case ro_top_to_bottom:
			reorder_top_to_bottom(tiles);
			break;
		case ro_random:
			reorder_random(tiles);
			break;
		default:
			break;
	}
}
