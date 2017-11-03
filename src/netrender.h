//
//  netrender.h
//  C-Ray
//
//  Created by Valtteri Koskivuori on 03/06/2017.
//  Copyright © 2017 Valtteri Koskivuori. All rights reserved.
//

/*
 The C-Ray NetRender network protocol implementation.
 Ideally all the related communication funcs will be here.
 */

#pragma once

#include <sys/socket.h>
#include <netinet/in.h>

struct remoteClient {
	
	int coreCount;
	int activeThreads;
	bool isRendering;
	
};

/*struct renderer {
	struct threadInfo *renderThreadInfo;
#ifndef WINDOWS
	pthread_attr_t renderThreadAttributes;
#endif
	struct scene *worldScene;
	struct renderTile *renderTiles;
	enum fileMode mode;
	int tileCount;
	int renderedTileCount;
	double *renderBuffer;
	unsigned char *uiBuffer;
	int threadCount;
	int activeThreads;
	bool isRendering;
	bool renderAborted;
	bool smoothShading;
	time_t avgTileTime;
	int timeSampleCount;
};*/
