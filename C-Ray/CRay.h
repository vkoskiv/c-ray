//
//  CRay.h
//  
//
//  Created by Valtteri Koskivuori on 12/02/15.
//
//

#ifndef ____CRay__
#define ____CRay__

#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <stdbool.h> //Need this for boolean data type
#include <math.h>
#include <string.h>
#include "vector.h"
#include "color.h"

//Some macros
#define PIOVER180 0.017453292519943295769236907684886
#define PI        3.141592653589793238462643383279502
#define min(a,b) (((a) < (b)) ? (a) : (b))
#define max(a,b) (((a) > (b)) ? (a) : (b))
#define invsqrtf(x) (1.0f / sqrtf(x))

//Image dimensions. Eventually get this from the input file
#define kImgWidth 1920
#define kImgHeight 1080
#define kFrameCount 1
#define bounces 3
#define contrast 1.0

//Object, a cube (WIP)
typedef struct {
	vector pos;
	float edgeLength;
	int material;
}cubeObject;

//Simulated light ray
typedef struct {
	vector start;
	vector direction;
}lightRay;

//material
typedef struct {
	color diffuse;
	float reflectivity;
}material;

//Light source
typedef struct {
	vector pos;
	float radius;
	color intensity;
}lightSource;

typedef struct {
#define conic 0
#define ortho 1
	unsigned char projectionType;
	double FOV;
}perspective;

//Generates a random value between a given range
float randRange(float a, float b);
//Calculates approximation of an inverse square root faster
float FastInvSqrt(float x);

#endif /* defined(____CRay__) */
