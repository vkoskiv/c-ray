//
//  tile.h
//  C-ray
//
//  Created by Valtteri Koskivuori on 06/07/2018.
//  Copyright © 2015-2019 Valtteri Koskivuori. All rights reserved.
//

#pragma once

struct renderer;

/**
 Render tile, contains needed information for the renderer
 */
struct renderTile {
	int width;
	int height;
	ivec2 begin;
	ivec2 end;
	int completedSamples;
	bool isRendering;
	bool renderComplete;
	bool hasHitObject;  //If a tile contains just ambient, we can skip tiles pretty safely after 25 samples or so.
	int tileNum;
};

struct texture;
enum renderOrder;

int quantizeImage(struct renderTile **renderTiles, struct texture *image, int tileWidth, int tileHeight);

void reorderTiles(struct renderTile **tiles, int tileCount, enum renderOrder tileOrder);

struct renderTile getTile(struct renderer *r);
