//
//  sphere.h
//  C-ray
//
//  Created by Valtteri Koskivuori on 28/02/2015.
//  Copyright © 2015-2020 Valtteri Koskivuori. All rights reserved.
//

#pragma once

#include "vector.h"
#include "material.h"

//Sphere
struct sphere {
	float radius;
	const struct bsdfNode *bsdf;
	struct color emission;
	float rayOffset;
};

struct sphere defaultSphere(void);

//Calculates intersection between a light ray and a sphere
bool rayIntersectsWithSphere(const struct lightRay *ray, const struct sphere *sphere, struct hitRecord *isect);
