//
//  renderer.h
//  C-Ray
//
//  Created by Valtteri Koskivuori on 19/02/2017.
//  Copyright © 2017 Valtteri Koskivuori. All rights reserved.
//

#pragma once

struct world;
enum renderOrder;
struct outputImage;

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

enum renderOrder {
	renderOrderTopToBottom = 0,
	renderOrderFromMiddle,
	renderOrderToMiddle,
	renderOrderNormal,
	renderOrderRandom
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
	struct world *scene; //Scene to render
	char *inputFilePath; //Directory to load input files from
	struct outputImage *image; //Output image
	struct renderTile *renderTiles; //Array of renderTiles to render
	int tileCount; //Total amount of render tiles
	enum fileMode mode;
	int renderedTileCount; //Completed render tiles
	double *renderBuffer;  //Double-precision buffer for multisampling
	unsigned char *uiBuffer; //UI element buffer
	int activeThreads; //Amount of threads currently rendering
	bool isRendering;
	bool renderPaused; //SDL listens for P key pressed, which sets this
	bool renderAborted;//SDL listens for X key pressed, which sets this
	bool smoothShading;//Unused
	time_t avgTileTime;//Used for render duration estimation
	int timeSampleCount;//Used for render duration estimation, amount of time samples captured
	
	//Prefs
	int threadCount; //Amount of threads to render with
	int sampleCount;
	bool antialiasing;
	bool newRenderer;
	int tileWidth;
	int tileHeight;
	enum renderOrder tileOrder;
};

/*
 Move to renderer:
 
 Move to UI:
 isFullScreen
 isBorderless
 windowScale
 */

//Renderer
#ifdef WINDOWS
DWORD WINAPI renderThread(LPVOID arg);
#else
void *renderThread(void *arg);
#endif
void quantizeImage();
void reorderTiles(enum renderOrder order);
