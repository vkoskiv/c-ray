//
//  tile.h
//  C-ray
//
//  Created by Valtteri Koskivuori on 06/07/2018.
//  Copyright © 2015-2020 Valtteri Koskivuori. All rights reserved.
//

#pragma once

struct renderer;

/**
 Render tile, contains needed information for the renderer
 */
struct renderTile {
	unsigned width;
	unsigned height;
	struct intCoord begin;
	struct intCoord end;
	int completedSamples;
	bool isRendering;
	bool renderComplete;
	bool hasHitObject;  //If a tile contains just ambient, we can skip tiles pretty safely after 25 samples or so.
	int tileNum;
};

struct texture;
enum renderOrder;

int quantizeImage(struct renderTile **renderTiles, struct texture *image, unsigned tileWidth, unsigned tileHeight);

void reorderTiles(struct renderTile **tiles, int tileCount, enum renderOrder tileOrder);

struct renderTile getTile(struct renderer *r);
