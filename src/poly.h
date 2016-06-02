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
	vector v1, v2, v3; //Three vertices
	int material;
    bool active;
}polygonObject;

typedef struct {
    vector v1, v2, v3, v4;
    int material;
    bool active;
}quadObject;

typedef struct {
    int polyCount;
    polygonObject *polyArray;
    int material;
}polyMesh;

//Calculates intersection between a light ray and a polygon object. Returns true if intersection has happened.
bool rayIntersectsWithPolygon(lightRay *r, polygonObject *t, double *result, vector *normal);

#endif /* defined(__C_Ray__poly__) */
