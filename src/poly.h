//
//  poly.h
//  C-Ray
//
//  Created by Valtteri Koskivuori on 28/02/15.
//  Copyright (c) 2015 Valtteri Koskivuori. All rights reserved.
//

#pragma once

struct poly {
	int vertexIndex[MAX_CRAY_VERTEX_COUNT];
	int normalIndex[MAX_CRAY_VERTEX_COUNT];
	int textureIndex[MAX_CRAY_VERTEX_COUNT];
	int materialIndex;
	int vertexCount;
};

//Main polygon array
extern struct poly *polygonArray;
extern int polyCount;

struct lightRay;

//Calculates intersection between a light ray and a polygon object. Returns true if intersection has happened.
bool rayIntersectsWithPolygon(struct lightRay *ray, struct poly *poly, double *result, struct vector *normal);

//Just check for intersection
bool rayIntersectsWithPolygonFast(struct lightRay *ray, struct poly *poly);
