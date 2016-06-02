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

typedef struct {
#define conic 0
#define ortho 1
	unsigned char projectionType;
	double FOV;
}perspective;

typedef struct {
#define ppm 0
#define bmp 1
#define png 2
	int height, width;
    unsigned char outputFileType;
	perspective viewPerspective;
	vector pos;
	vector lookAt;
    bool antialiased;
    int supersampling;
    int frameCount;
    int currentFrame;
	int bounces;
	float contrast;
}camera;

#endif /* defined(__C_Ray__camera__) */
