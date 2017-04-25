//
//  poly.h
//  C-Ray
//
//  Created by Valtteri Koskivuori on 28/02/15.
//  Copyright (c) 2015 Valtteri Koskivuori. All rights reserved.
//

#pragma once

#include "includes.h"
#include "vector.h"


typedef struct {
	int vertexIndex[MAX_CRAY_VERTEX_COUNT];
	int normalIndex[MAX_CRAY_VERTEX_COUNT];
	int textureIndex[MAX_CRAY_VERTEX_COUNT];
	int materialIndex;
	int vertexCount;
}poly;

//Main polygon array
extern poly *polygonArray;
extern int polyCount;

//Calculates intersection between a light ray and a polygon object. Returns true if intersection has happened.
bool rayIntersectsWithPolygon(lightRay *ray, poly *poly, double *result, vector *normal);

//Just check for intersection
bool rayIntersectsWithPolygonFast(lightRay *ray, poly *poly);
