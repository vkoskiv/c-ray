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
	struct intCoord begin;
	struct intCoord end;
	int completedSamples;
	bool isRendering;
	bool renderComplete;
	int tileNum;
};

struct texture;
enum renderOrder;

int quantizeImage(struct renderTile **renderTiles, struct texture *image, int tileWidth, int tileHeight);

void reorderTiles(struct renderTile **tiles, int tileCount, enum renderOrder tileOrder);

struct renderTile getTile(struct renderer *r);
