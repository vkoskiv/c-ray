//
//  sphere.h
//  C-Ray
//
//  Created by Valtteri Koskivuori on 28/02/15.
//  Copyright (c) 2015 Valtteri Koskivuori. All rights reserved.
//

#ifndef __C_Ray__sphere__
#define __C_Ray__sphere__

#include "includes.h"
#include "vector.h"

//Object, a sphere in this case
typedef struct {
	vector pos;
	float radius;
	int material;
    bool active;
}sphereObject;

//Calculates intersection between a light ray and a sphere object;
bool rayIntersectsWithSphere(lightRay *ray, sphereObject *sphere, double *t);

#endif /* defined(__C_Ray__sphere__) */
