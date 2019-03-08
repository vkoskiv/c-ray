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
	struct coord begin;
	struct coord end;
	int completedSamples;
	bool isRendering;
	int tileNum;
};

struct texture;
enum renderOrder;

int quantizeImage(struct renderTile **renderTiles, struct texture *image, int tileWidth, int tileHeight);

void reorderTiles(struct renderTile **tiles, int tileCount, enum renderOrder tileOrder, pcg32_random_t *rng);

struct renderTile getTile(struct renderer *r);
