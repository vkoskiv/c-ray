//
//  camera.h
//  C-Ray
//
//  Created by Valtteri Koskivuori on 02/03/15.
//  Copyright (c) 2015 Valtteri Koskivuori. All rights reserved.
//

#ifndef __C_Ray__camera__
#define __C_Ray__camera__

#include "includes.h"
#include "vector.h"

typedef enum {
	conic,
	ortho
}projectionType;

typedef struct {
	projectionType projectionType;
	double FOV;
}perspective;

typedef enum {
	bmp,
	png
}fileType;

typedef struct {
	int height, width;
	fileType fileType;
	perspective viewPerspective;
	vector pos;
	vector lookAt;
	vector up;
	vector u, v, w;
	unsigned char *imgData;
	bool areaLights;
	bool approximateMeshShadows;
	bool forceSingleCore;
	bool showGUI;
	float focalLength;
	int sampleCount;
	int threadCount;
	int frameCount;
	int currentFrame;
	int bounces;
	float contrast;
	float windowScale;
	
	bool useTiles;
	int tileWidth;
	int tileHeight;
}camera;

#endif /* defined(__C_Ray__camera__) */
