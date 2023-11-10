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
#include "../utils/dyn_array.h"

struct sphere {
	float radius;
	float rayOffset;
};

typedef struct sphere sphere;
dyn_array_dec(sphere);

bool rayIntersectsWithSphere(const struct lightRay *ray, const struct sphere *sphere, struct hitRecord *isect);
