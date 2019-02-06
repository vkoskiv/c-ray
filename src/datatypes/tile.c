//
//  tile.c
//  C-ray
//
//  Created by Valtteri Koskivuori on 06/07/2018.
//  Copyright © 2015-2019 Valtteri Koskivuori. All rights reserved.
//

#include "../includes.h"
#include "tile.h"

#include "../renderer/renderer.h"
#include "../utils/logging.h"
#include "../utils/filehandler.h"

void reorderTiles(struct renderer *r);

/**
 Gets the next tile from renderTiles in mainRenderer
 
 @return A renderTile to be rendered
 */
struct renderTile getTile(struct renderer *r) {
	struct renderTile tile;
	memset(&tile, 0, sizeof(tile));
	tile.tileNum = -1;
#ifdef WINDOWS
	WaitForSingleObject(r->tileMutex, INFINITE);
#else
	pthread_mutex_lock(&r->tileMutex);
#endif
	if (r->finishedTileCount < r->tileCount) {
		tile = r->renderTiles[r->finishedTileCount];
		r->renderTiles[r->finishedTileCount].isRendering = true;
		tile.tileNum = r->finishedTileCount++;
	}
#ifdef WINDOWS
	ReleaseMutex(r->tileMutex);
#else
	pthread_mutex_unlock(&r->tileMutex);
#endif
	return tile;
}

/**
 Create tiles from render plane, and add those to mainRenderer
 
 @param scene scene object
 */
void quantizeImage(struct renderer *r) {
	
	logr(info, "Quantizing render plane...\n");
	
	//Sanity check on tilesizes
	if (r->tileWidth >= r->image->size.width) r->tileWidth = r->image->size.width;
	if (r->tileHeight >= r->image->size.height) r->tileHeight = r->image->size.height;
	if (r->tileWidth <= 0) r->tileWidth = 1;
	if (r->tileHeight <= 0) r->tileHeight = 1;
	
	int tilesX = r->image->size.width / r->tileWidth;
	int tilesY = r->image->size.height / r->tileHeight;
	
	tilesX = (r->image->size.width % r->tileWidth) != 0 ? tilesX + 1: tilesX;
	tilesY = (r->image->size.height % r->tileHeight) != 0 ? tilesY + 1: tilesY;
	
	r->renderTiles = calloc(tilesX*tilesY, sizeof(struct renderTile));
	if (r->renderTiles == NULL) {
		logr(error, "Failed to allocate renderTiles array.\n");
	}
	
	for (int y = 0; y < tilesY; y++) {
		for (int x = 0; x < tilesX; x++) {
			struct renderTile *tile = &r->renderTiles[x + y*tilesX];
			tile->width  = r->tileWidth;
			tile->height = r->tileHeight;
			
			tile->begin.x = x       * r->tileWidth;
			tile->end.x   = (x + 1) * r->tileWidth;
			
			tile->begin.y = y       * r->tileHeight;
			tile->end.y   = (y + 1) * r->tileHeight;
			
			tile->end.x = min((x + 1) * r->tileWidth, r->image->size.width);
			tile->end.y = min((y + 1) * r->tileHeight, r->image->size.height);
			
			//Samples have to start at 1, so the running average works
			tile->completedSamples = 1;
			tile->isRendering = false;
			tile->tileNum = r->tileCount;
			
			r->tileCount++;
		}
	}
	logr(info, "Quantized image into %i tiles. (%ix%i)\n", (tilesX*tilesY), tilesX, tilesY);
	
	reorderTiles(r);
}

/**
 Reorder renderTiles to start from top
 */
void reorderTopToBottom(struct renderer *r) {
	int endIndex = r->tileCount - 1;
	
	struct renderTile *tempArray = calloc(r->tileCount, sizeof(struct renderTile));
	
	for (int i = 0; i < r->tileCount; i++) {
		tempArray[i] = r->renderTiles[endIndex--];
	}
	
	free(r->renderTiles);
	r->renderTiles = tempArray;
}

unsigned int rand_interval(unsigned int min, unsigned int max) {
	unsigned int r;
	const unsigned int range = 1 + max - min;
	const unsigned int buckets = RAND_MAX / range;
	const unsigned int limit = buckets * range;
	
	/* Create equal size buckets all in a row, then fire randomly towards
	 * the buckets until you land in one of them. All buckets are equally
	 * likely. If you land off the end of the line of buckets, try again. */
	do
	{
		r = rand();
	} while (r >= limit);
	
	return min + (r / buckets);
}

/**
 Shuffle renderTiles into a random order
 */
void reorderRandom(struct renderer *r) {
	for (int i = 0; i < r->tileCount; i++) {
		unsigned int random = rand_interval(0, r->tileCount - 1);
		
		struct renderTile temp = r->renderTiles[i];
		r->renderTiles[i] = r->renderTiles[random];
		r->renderTiles[random] = temp;
	}
}

/**
 Reorder renderTiles to start from middle
 */
void reorderFromMiddle(struct renderer *r) {
	int midLeft = 0;
	int midRight = 0;
	bool isRight = true;
	
	midRight = ceil(r->tileCount / 2);
	midLeft = midRight - 1;
	
	struct renderTile *tempArray = calloc(r->tileCount, sizeof(struct renderTile));
	
	for (int i = 0; i < r->tileCount; i++) {
		if (isRight) {
			tempArray[i] = r->renderTiles[midRight++];
			isRight = false;
		} else {
			tempArray[i] = r->renderTiles[midLeft--];
			isRight = true;
		}
	}
	
	free(r->renderTiles);
	r->renderTiles = tempArray;
}


/**
 Reorder renderTiles to start from ends, towards the middle
 */
void reorderToMiddle(struct renderer *r) {
	int left = 0;
	int right = 0;
	bool isRight = true;
	
	right = r->tileCount - 1;
	
	struct renderTile *tempArray = calloc(r->tileCount, sizeof(struct renderTile));
	
	for (int i = 0; i < r->tileCount; i++) {
		if (isRight) {
			tempArray[i] = r->renderTiles[right--];
			isRight = false;
		} else {
			tempArray[i] = r->renderTiles[left++];
			isRight = true;
		}
	}
	
	free(r->renderTiles);
	r->renderTiles = tempArray;
}

/**
 Reorder renderTiles in given order
 
 @param order Render order to be applied
 */
void reorderTiles(struct renderer *r) {
	switch (r->tileOrder) {
		case renderOrderFromMiddle:
		{
			reorderFromMiddle(r);
		}
			break;
		case renderOrderToMiddle:
		{
			reorderToMiddle(r);
		}
			break;
		case renderOrderTopToBottom:
		{
			reorderTopToBottom(r);
		}
			break;
		case renderOrderRandom:
		{
			reorderRandom(r);
		}
			break;
		default:
			break;
	}
}
