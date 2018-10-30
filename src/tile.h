//
//  tile.h
//  C-Ray
//
//  Created by Valtteri on 06/07/2018.
//  Copyright © 2018 Valtteri Koskivuori. All rights reserved.
//

#pragma once

struct image;
struct renderer;
enum renderOrder;

/**
 Render tile, contains needed information for the renderer
 */
struct renderTile {
	int width;
	int height;
	struct coord begin;
	struct coord end;
	int completedSamples;
	bool isRendering;
	int tileNum;
};

int quantizeImage(struct renderTile **renderTiles, struct image *image, int tileWidth, int tileHeight);
void reorderTiles(struct renderTile **tiles, int tileCount, enum renderOrder tileOrder);

struct renderTile getTile(struct renderer *r);
