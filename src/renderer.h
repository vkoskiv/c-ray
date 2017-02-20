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

//Some macros
#define min(a,b) (((a) < (b)) ? (a) : (b))
#define max(a,b) (((a) > (b)) ? (a) : (b))
#define invsqrtf(x) (1.0f / sqrtf(x))

typedef struct {
	pthread_t thread_id;
	int thread_num;
	int completedSamples;
	bool threadComplete;
}threadInfo;

typedef struct {
	int width;
	int height;
	int startX, startY;
	int endX, endY;
	poly *polygons;
	sphere *spheres;
}renderTile;

typedef struct {
	threadInfo *renderThreadInfo;
	pthread_attr_t renderThreadAttributes;
	world *worldScene;
	renderTile *renderTiles;
	int tileCount;
	double *renderBuffer;
	int sectionSize; //Replace with array of renderTasks
	int threadCount;
	int activeThreads;
	bool shouldSave;
	bool isRendering;
}renderer;

//Renderer
extern renderer mainRenderer;

void *renderThread(void *arg);
void quantizeImage(world *worldScene);

#endif /* renderer_h */
