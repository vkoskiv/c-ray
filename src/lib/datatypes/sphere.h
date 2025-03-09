//
//  sphere.h
//  c-ray
//
//  Created by Valtteri Koskivuori on 28/02/2015.
//  Copyright © 2015-2025 Valtteri Koskivuori. All rights reserved.
//

#pragma once

#include <datatypes/lightray.h>
#include <datatypes/hitrecord.h>
#include <common/dyn_array.h>

struct sphere {
	float radius;
	float rayOffset;
};

typedef struct sphere sphere;
dyn_array_def(sphere)

bool rayIntersectsWithSphere(const struct lightRay *ray, const struct sphere *sphere, struct hitRecord *isect);
