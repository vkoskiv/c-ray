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

static void reorderTiles(struct renderTile **tiles, unsigned tileCount, enum renderOrder tileOrder);

struct renderTile *nextTile(struct renderer *r) {
	struct renderTile *tile = NULL;
	mutex_lock(r->state.tileMutex);
	if (r->state.finishedTileCount < r->state.tileCount) {
		tile = &r->state.renderTiles[r->state.finishedTileCount];
		tile->state = rendering;
		tile->tileNum = r->state.finishedTileCount++;
	} else {
		// If a network worker disappeared during render, finish those tiles locally here at the end
		for (int t = 0; t < r->state.tileCount; ++t) {
			if (r->state.renderTiles[t].state == rendering && r->state.renderTiles[t].networkRenderer) {
				r->state.renderTiles[t].networkRenderer = false;
				tile = &r->state.renderTiles[t];
				tile->state = rendering;
				tile->tileNum = t;
				break;
			}
		}
	}
	mutex_release(r->state.tileMutex);
	return tile;
}

struct renderTile *nextTileInteractive(struct renderer *r) {
	struct renderTile *tile = NULL;
	mutex_lock(r->state.tileMutex);
	again:
	if (r->state.finishedPasses < r->prefs.sampleCount + 1) {
		if (r->state.finishedTileCount < r->state.tileCount) {
			tile = &r->state.renderTiles[r->state.finishedTileCount];
			tile->state = rendering;
			tile->tileNum = r->state.finishedTileCount++;
		} else {
			r->state.finishedPasses++;
			r->state.finishedTileCount = 0;
			goto again;
		}
	}
	mutex_release(r->state.tileMutex);
	return tile;
}

unsigned quantizeImage(struct renderTile **renderTiles, unsigned width, unsigned height, unsigned tileWidth, unsigned tileHeight, enum renderOrder tileOrder) {
	
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
	
	*renderTiles = calloc(tilesX*tilesY, sizeof(**renderTiles));
	if (*renderTiles == NULL) {
		logr(error, "Failed to allocate renderTiles array.\n");
		return 0;
	}
	
	int tileCount = 0;
	for (unsigned y = 0; y < tilesY; ++y) {
		for (unsigned x = 0; x < tilesX; ++x) {
			struct renderTile *tile = &(*renderTiles)[x + y * tilesX];
			tile->width  = tileWidth;
			tile->height = tileHeight;
			
			tile->begin.x = x       * tileWidth;
			tile->end.x   = (x + 1) * tileWidth;
			
			tile->begin.y = y       * tileHeight;
			tile->end.y   = (y + 1) * tileHeight;
			
			tile->end.x = min((x + 1) * tileWidth, width);
			tile->end.y = min((y + 1) * tileHeight, height);
			
			tile->width = tile->end.x - tile->begin.x;
			tile->height = tile->end.y - tile->begin.y;

			tile->state = ready_to_render;
			//Samples have to start at 1, so the running average works
			tile->tileNum = tileCount++;
		}
	}
	logr(info, "Quantized image into %i tiles. (%ix%i)\n", (tilesX*tilesY), tilesX, tilesY);
	
	reorderTiles(renderTiles, tileCount, tileOrder);
	
	return tileCount;
}

static void reorderTopToBottom(struct renderTile **tiles, unsigned tileCount) {
	unsigned endIndex = tileCount - 1;
	
	struct renderTile *tempArray = calloc(tileCount, sizeof(*tempArray));
	
	for (unsigned i = 0; i < tileCount; ++i) {
		tempArray[i] = (*tiles)[endIndex--];
	}
	
	free(*tiles);
	*tiles = tempArray;
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

static void reorderRandom(struct renderTile **tiles, unsigned tileCount) {
	pcg32_random_t rng;
	pcg32_srandom_r(&rng, 3141592, 0);
	for (unsigned i = 0; i < tileCount; ++i) {
		unsigned random = rand_interval(0, tileCount - 1, &rng);
		
		struct renderTile temp = (*tiles)[i];
		(*tiles)[i] = (*tiles)[random];
		(*tiles)[random] = temp;
	}
}

static void reorderFromMiddle(struct renderTile **tiles, unsigned tileCount) {
	int midLeft = 0;
	int midRight = 0;
	bool isRight = true;
	
	midRight = ceil(tileCount / 2);
	midLeft = midRight - 1;
	
	struct renderTile *tempArray = calloc(tileCount, sizeof(*tempArray));
	
	for (unsigned i = 0; i < tileCount; ++i) {
		if (isRight) {
			tempArray[i] = (*tiles)[midRight++];
			isRight = false;
		} else {
			tempArray[i] = (*tiles)[midLeft--];
			isRight = true;
		}
	}
	
	free(*tiles);
	*tiles = tempArray;
}

static void reorderToMiddle(struct renderTile **tiles, unsigned tileCount) {
	unsigned left = 0;
	unsigned right = 0;
	bool isRight = true;
	
	right = tileCount - 1;
	
	struct renderTile *tempArray = calloc(tileCount, sizeof(*tempArray));
	
	for (unsigned i = 0; i < tileCount; ++i) {
		if (isRight) {
			tempArray[i] = (*tiles)[right--];
			isRight = false;
		} else {
			tempArray[i] = (*tiles)[left++];
			isRight = true;
		}
	}
	
	free(*tiles);
	*tiles = tempArray;
}

static void reorderTiles(struct renderTile **tiles, unsigned tileCount, enum renderOrder tileOrder) {
	switch (tileOrder) {
		case renderOrderFromMiddle:
			reorderFromMiddle(tiles, tileCount);
			break;
		case renderOrderToMiddle:
			reorderToMiddle(tiles, tileCount);
			break;
		case renderOrderTopToBottom:
			reorderTopToBottom(tiles, tileCount);
			break;
		case renderOrderRandom:
			reorderRandom(tiles, tileCount);
			break;
		default:
			break;
	}
}
