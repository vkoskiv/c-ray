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
	bmp,
	png
}fileType;

typedef enum {
	renderOrderTopToBottom = 0,
	renderOrderFromMiddle,
	renderOrderNormal
}renderOrder;

typedef struct {
	int height, width; // Image dimensions
	fileType fileType;
	
	double FOV;
	double focalLength;
	double aperture;
	vector pos;
	
	unsigned char *imgData;
	bool areaLights;
	bool aprxShadows;
	int sampleCount;
	int threadCount;
	renderOrder tileOrder;
	int frameCount;
	int currentFrame;
	int bounces;
	float contrast;
	float windowScale;
	
	bool isFullScreen;
	bool isBorderless;
	
	bool useTiles;
	int tileWidth;
	int tileHeight;
}camera;

//Calculate camera view plane
void calculateUVW(camera *camera);

#endif /* defined(__C_Ray__camera__) */
