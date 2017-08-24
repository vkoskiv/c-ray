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


/**
 Thread information struct to communicate with main thread
 */
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


/**
 Render tile, contains needed information for the renderer
 */
struct renderTile {
	int width;
	int height;
	//TODO: Consider position struct for these
	int startX, startY;
	int endX, endY;
	int completedSamples;
	bool isRendering;
	int tileNum;
	time_t start, stop;
};


/**
 Main renderer. Stores needed information to keep track of render status,
 as well as information needed for the rendering routines.
 */
struct renderer {
	struct threadInfo *renderThreadInfo; //Info about threads
#ifndef WINDOWS
	pthread_attr_t renderThreadAttributes;
#endif
	struct scene *worldScene; //Scene to render
	struct renderTile *renderTiles; //Array of renderTiles to render
	int tileCount; //Total amount of render tiles
	enum fileMode mode;
	int renderedTileCount; //Completed render tiles
	double *renderBuffer;  //Double-precision buffer for multisampling
	unsigned char *uiBuffer;//UI element buffer
	int threadCount;
	int activeThreads;
	bool isRendering;
	bool renderAborted;
	bool smoothShading;
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
