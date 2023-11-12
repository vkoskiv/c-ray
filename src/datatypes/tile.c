//
//  tile.c
//  C-ray
//
//  Created by Valtteri Koskivuori on 06/07/2018.
//  Copyright © 2018-2022 Valtteri Koskivuori. All rights reserved.
//

#include "../includes.h"
#include "../datatypes/image/imagefile.h"
#include "../renderer/renderer.h"
#include "tile.h"

#include "../utils/logging.h"
#include "../utils/platform/mutex.h"
#include "../vendored/pcg_basic.h"
#include <string.h>

static void tiles_reorder(struct render_tile_arr *tiles, enum render_order tileOrder);

struct render_tile *tile_next(struct renderer *r) {
	struct render_tile *tile = NULL;
	mutex_lock(r->state.tileMutex);
	if (r->state.finishedTileCount < r->state.tiles.count) {
		tile = &r->state.tiles.items[r->state.finishedTileCount];
		tile->state = rendering;
		tile->index = r->state.finishedTileCount++;
	} else {
		// If a network worker disappeared during render, finish those tiles locally here at the end
		for (size_t t = 0; t < r->state.tiles.count; ++t) {
			if (r->state.tiles.items[t].state == rendering && r->state.tiles.items[t].network_renderer) {
				r->state.tiles.items[t].network_renderer = false;
				tile = &r->state.tiles.items[t];
				tile->state = rendering;
				tile->index = t;
				break;
			}
		}
	}
	mutex_release(r->state.tileMutex);
	return tile;
}

struct render_tile *tile_next_interactive(struct renderer *r) {
	struct render_tile *tile = NULL;
	mutex_lock(r->state.tileMutex);
	again:
	if (r->state.finishedPasses < r->prefs.sampleCount + 1) {
		if (r->state.finishedTileCount < r->state.tiles.count) {
			tile = &r->state.tiles.items[r->state.finishedTileCount];
			tile->state = rendering;
			tile->index = r->state.finishedTileCount++;
		} else {
			r->state.finishedPasses++;
			r->state.finishedTileCount = 0;
			goto again;
		}
	}
	mutex_release(r->state.tileMutex);
	return tile;
}

unsigned tile_quantize(struct render_tile_arr *tiles, unsigned width, unsigned height, unsigned tileWidth, unsigned tileHeight, enum render_order tileOrder) {
	
	logr(info, "Quantizing render plane\n");
	
	//Sanity check on tilesizes
	if (tileWidth >= width) tileWidth = width;
	if (tileHeight >= height) tileHeight = height;
	if (tileWidth <= 0) tileWidth = 1;
	if (tileHeight <= 0) tileHeight = 1;
	
	unsigned tilesX = width / tileWidth;
	unsigned tilesY = height / tileHeight;
	
	tilesX = (width % tileWidth) != 0 ? tilesX + 1: tilesX;
	tilesY = (height % tileHeight) != 0 ? tilesY + 1: tilesY;

	int tileCount = 0;
	for (unsigned y = 0; y < tilesY; ++y) {
		for (unsigned x = 0; x < tilesX; ++x) {
			struct render_tile tile = { 0 };
			tile.width  = tileWidth;
			tile.height = tileHeight;
			
			tile.begin.x = x       * tileWidth;
			tile.end.x   = (x + 1) * tileWidth;
			
			tile.begin.y = y       * tileHeight;
			tile.end.y   = (y + 1) * tileHeight;
			
			tile.end.x = min((x + 1) * tileWidth, width);
			tile.end.y = min((y + 1) * tileHeight, height);
			
			tile.width = tile.end.x - tile.begin.x;
			tile.height = tile.end.y - tile.begin.y;

			tile.state = ready_to_render;

			tile.index = tileCount++;
			render_tile_arr_add(tiles, tile);
		}
	}
	logr(info, "Quantized image into %i tiles. (%ix%i)\n", (tilesX*tilesY), tilesX, tilesY);
	
	tiles_reorder(tiles, tileOrder);
	
	return tileCount;
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
