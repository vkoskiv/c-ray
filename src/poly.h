//
//  poly.h
//  C-Ray
//
//  Created by Valtteri Koskivuori on 28/02/15.
//  Copyright (c) 2015 Valtteri Koskivuori. All rights reserved.
//

#ifndef __C_Ray__poly__
#define __C_Ray__poly__

#include "includes.h"
#include "vector.h"

typedef struct {
	int vertexIndex[MAX_VERTEX_COUNT];
	int normalIndex[MAX_VERTEX_COUNT];
	int textureIndex[MAX_VERTEX_COUNT];
	int materialIndex;
	int vertexCount;
    bool active;
}poly;

//Calculates intersection between a light ray and a polygon object. Returns true if intersection has happened.
bool rayIntersectsWithPolygon(lightRay *ray, poly *poly, double *result, vector *normal);

#endif /* defined(__C_Ray__poly__) */
