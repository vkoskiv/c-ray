//
//  tile.c
//  C-ray
//
//  Created by Valtteri Koskivuori on 06/07/2018.
//  Copyright © 2015-2020 Valtteri Koskivuori. All rights reserved.
//

#include "../includes.h"
#include "tile.h"

#include "../renderer/renderer.h"
#include "../utils/logging.h"
#include "../datatypes/texture.h"

/**
 Gets the next tile from renderTiles in mainRenderer
 
 @return A renderTile to be rendered
 */
struct renderTile getTile(struct renderer *r) {
	struct renderTile tile;
	memset(&tile, 0, sizeof(tile));
	tile.tileNum = -1;
#ifdef WINDOWS
	WaitForSingleObject(r->state.tileMutex, INFINITE);
#else
	pthread_mutex_lock(&r->state.tileMutex);
#endif
	if (r->state.finishedTileCount < r->state.tileCount) {
		tile = r->state.renderTiles[r->state.finishedTileCount];
		r->state.renderTiles[r->state.finishedTileCount].isRendering = true;
		tile.tileNum = r->state.finishedTileCount++;
	}
#ifdef WINDOWS
	ReleaseMutex(r->state.tileMutex);
#else
	pthread_mutex_unlock(&r->state.tileMutex);
#endif
	return tile;
}

/**
 Create tiles from render plane, and add those to mainRenderer
 
 @param scene scene object
 */
int quantizeImage(struct renderTile **renderTiles, unsigned width, unsigned height, unsigned tileWidth, unsigned tileHeight) {
	
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
	
	*renderTiles = calloc(tilesX*tilesY, sizeof(struct renderTile));
	if (*renderTiles == NULL) {
		logr(error, "Failed to allocate renderTiles array.\n");
		return 0;
	}
	
	unsigned tileCount = 0;
	for (unsigned y = 0; y < tilesY; y++) {
		for (unsigned x = 0; x < tilesX; x++) {
			struct renderTile *tile = &(*renderTiles)[x + y*tilesX];
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
			tile->tileNum = tileCount;
			
			tileCount++;
		}
	}
	logr(info, "Quantized image into %i tiles. (%ix%i)\n", (tilesX*tilesY), tilesX, tilesY);
	
	return tileCount;
}

/**
 Reorder renderTiles to start from top
 */
void reorderTopToBottom(struct renderTile **tiles, int tileCount) {
	int endIndex = tileCount - 1;
	
	struct renderTile *tempArray = calloc(tileCount, sizeof(struct renderTile));
	
	for (int i = 0; i < tileCount; i++) {
		tempArray[i] = (*tiles)[endIndex--];
	}
	
	free(*tiles);
	*tiles = tempArray;
}

unsigned int rand_interval(unsigned int min, unsigned int max, pcg32_random_t *rng) {
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
void reorderRandom(struct renderTile **tiles, int tileCount) {
	pcg32_random_t *rng = calloc(1, sizeof(pcg32_random_t));
	pcg32_srandom_r(rng, 3141592, 0);
	for (int i = 0; i < tileCount; i++) {
		unsigned int random = rand_interval(0, tileCount - 1, rng);
		
		struct renderTile temp = (*tiles)[i];
		(*tiles)[i] = (*tiles)[random];
		(*tiles)[random] = temp;
	}
	free(rng);
}

/**
 Reorder renderTiles to start from middle
 */
void reorderFromMiddle(struct renderTile **tiles, int tileCount) {
	int midLeft = 0;
	int midRight = 0;
	bool isRight = true;
	
	midRight = ceil(tileCount / 2);
	midLeft = midRight - 1;
	
	struct renderTile *tempArray = calloc(tileCount, sizeof(struct renderTile));
	
	for (int i = 0; i < tileCount; i++) {
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
void reorderToMiddle(struct renderTile **tiles, int tileCount) {
	int left = 0;
	int right = 0;
	bool isRight = true;
	
	right = tileCount - 1;
	
	struct renderTile *tempArray = calloc(tileCount, sizeof(struct renderTile));
	
	for (int i = 0; i < tileCount; i++) {
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
void reorderTiles(struct renderTile **tiles, int tileCount, enum renderOrder tileOrder) {
	switch (tileOrder) {
		case renderOrderFromMiddle:
		{
			reorderFromMiddle(tiles, tileCount);
		}
			break;
		case renderOrderToMiddle:
		{
			reorderToMiddle(tiles, tileCount);
		}
			break;
		case renderOrderTopToBottom:
		{
			reorderTopToBottom(tiles, tileCount);
		}
			break;
		case renderOrderRandom:
		{
			reorderRandom(tiles, tileCount);
		}
			break;
		default:
			break;
	}
}
