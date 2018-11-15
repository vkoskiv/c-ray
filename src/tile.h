//
//  tile.h
//  C-ray
//
//  Created by Valtteri Koskivuori on 06/07/2018.
//  Copyright © 2015-2018 Valtteri Koskivuori. All rights reserved.
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

void quantizeImage(struct renderer *renderer);

struct renderTile getTile(struct renderer *r);
