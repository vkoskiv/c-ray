//
//  renderer.h
//  C-Ray
//
//  Created by Valtteri Koskivuori on 19/02/2017.
//  Copyright © 2017 Valtteri Koskivuori. All rights reserved.
//

#pragma once

struct scene;
enum renderOrder;

struct threadInfo {
#ifdef WINDOWS
	HANDLE thread_handle;
	DWORD thread_id;
#else
	pthread_t thread_id;
#endif
	int thread_num;
	bool threadComplete;
};

struct renderTile {
	int width;
	int height;
	int startX, startY;
	int endX, endY;
	int completedSamples;
	bool isRendering;
	int tileNum;
	time_t start, stop;
};

struct renderer {
	struct threadInfo *renderThreadInfo;
#ifndef WINDOWS
	pthread_attr_t renderThreadAttributes;
#endif
	struct scene *worldScene;
	struct renderTile *renderTiles;
	int tileCount;
	int renderedTileCount;
	double *renderBuffer;
	unsigned char *uiBuffer;
	int threadCount;
	int activeThreads;
	bool shouldSave;
	bool isRendering;
	bool renderAborted;
	time_t avgTileTime;
	int timeSampleCount;
};

//Renderer
#ifdef WINDOWS
DWORD WINAPI renderThread(LPVOID arg);
#else
void *renderThread(void *arg);
#endif
void quantizeImage(struct scene *worldScene);
void reorderTiles(enum renderOrder order);
