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

//Sphere
typedef struct {
	vector pos;
	float radius;
	int material;
    bool active;
}sphere;

//Calculates intersection between a light ray and a sphere
bool rayIntersectsWithSphere(lightRay *ray, sphere *sphere, double *t);

#endif /* defined(__C_Ray__sphere__) */
