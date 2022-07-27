//
//  tile.h
//  C-ray
//
//  Created by Valtteri Koskivuori on 06/07/2018.
//  Copyright © 2018-2022 Valtteri Koskivuori. All rights reserved.
//

#pragma once

#include "../includes.h"

#include "vector.h"

enum renderOrder {
	renderOrderTopToBottom = 0,
	renderOrderFromMiddle,
	renderOrderToMiddle,
	renderOrderNormal,
	renderOrderRandom
};

struct renderer;

enum tile_state {
	ready_to_render,
	rendering,
	finished
};

/**
 Render tile, contains needed information for the renderer
 */
struct renderTile {
	unsigned width;
	unsigned height;
	struct intCoord begin;
	struct intCoord end;
	enum tile_state state;
	bool networkRenderer;
	int tileNum;
};

/// Quantize the render plane into an array of tiles, with properties as specified in the parameters below
/// @param renderTiles Array to place renderTiles into
/// @param width Render plane width
/// @param height Render plane height
/// @param tileWidth Tile width
/// @param tileHeight Tile height
/// @param tileOrder Order for the renderer to render the tiles in
unsigned quantizeImage(struct renderTile **renderTiles, unsigned width, unsigned height, unsigned tileWidth, unsigned tileHeight, enum renderOrder tileOrder);


/// Grab the next tile from the queue
/// @param r It's the renderer, yo.
struct renderTile *nextTile(struct renderer *r);

struct renderTile *nextTileInteractive(struct renderer *r);
