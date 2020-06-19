//
//  tile.c
//  C-ray
//
//  Created by Valtteri Koskivuori on 06/07/2018.
//  Copyright © 2015-2020 Valtteri Koskivuori. All rights reserved.
//

#include "../includes.h"
#include "tile.h"

#include "../datatypes/image/imagefile.h"
#include "../renderer/renderer.h"
#include "../utils/logging.h"
#include "../utils/platform/mutex.h"

static void reorderTiles(struct renderTile **tiles, unsigned tileCount, enum renderOrder tileOrder);

/**
 Gets the next tile from renderTiles in mainRenderer
 
 @return A renderTile to be rendered
 */
struct renderTile nextTile(struct renderer *r) {
	struct renderTile tile;
	memset(&tile, 0, sizeof(tile));
	tile.tileNum = -1;
	lockMutex(r->state.tileMutex);
	if (r->state.finishedTileCount < r->state.tileCount) {
		tile = r->state.renderTiles[r->state.finishedTileCount];
		r->state.renderTiles[r->state.finishedTileCount].isRendering = true;
		tile.tileNum = r->state.finishedTileCount++;
	}
	releaseMutex(r->state.tileMutex);
	return tile;
}

/**
 Create tiles from render plane, and add those to mainRenderer
 
 @param scene scene object
 */
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
			
			//Samples have to start at 1, so the running average works
			tile->completedSamples = 1;
			tile->isRendering = false;
			tile->tileNum = tileCount++;
		}
	}
	logr(info, "Quantized image into %i tiles. (%ix%i)\n", (tilesX*tilesY), tilesX, tilesY);
	
	reorderTiles(renderTiles, tileCount, tileOrder);
	
	return tileCount;
}

/**
 Reorder renderTiles to start from top
 */
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

/**
 Shuffle renderTiles into a random order
 */
static void reorderRandom(struct renderTile **tiles, unsigned tileCount) {
	pcg32_random_t *rng = calloc(1, sizeof(*rng));
	pcg32_srandom_r(rng, 3141592, 0);
	for (unsigned i = 0; i < tileCount; ++i) {
		unsigned random = rand_interval(0, tileCount - 1, rng);
		
		struct renderTile temp = (*tiles)[i];
		(*tiles)[i] = (*tiles)[random];
		(*tiles)[random] = temp;
	}
	free(rng);
}

/**
 Reorder renderTiles to start from middle
 */
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

/**
 Reorder renderTiles to start from ends, towards the middle
 */
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

/**
 Reorder renderTiles in given order
 
 @param order Render order to be applied
 */
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
