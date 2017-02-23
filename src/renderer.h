//
//  renderer.h
//  C-Ray
//
//  Created by Valtteri Koskivuori on 19/02/2017.
//  Copyright © 2017 Valtteri Koskivuori. All rights reserved.
//

#ifndef renderer_h
#define renderer_h

#include "includes.h"
#include "vector.h"
#include "color.h"
#include "scene.h"
#include "poly.h"

typedef struct {
	pthread_t thread_id;
	int thread_num;
	bool threadComplete;
}threadInfo;

typedef struct {
	int width;
	int height;
	int startX, startY;
	int endX, endY;
	int completedSamples;
	bool isRendering;
	int tileNum;
}renderTile;

typedef struct {
	threadInfo *renderThreadInfo;
	pthread_attr_t renderThreadAttributes;
	world *worldScene;
	renderTile *renderTiles;
	int tileCount;
	int renderedTileCount;
	double *renderBuffer;
	unsigned char *uiBuffer;
	int threadCount;
	int activeThreads;
	bool shouldSave;
	bool isRendering;
	bool renderAborted;
}renderer;

//Renderer
extern renderer mainRenderer;

void *renderThread(void *arg);
void quantizeImage(world *worldScene);

#endif /* renderer_h */
