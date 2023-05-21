//
//  lightRay.h
//  C-ray
//
//  Created by Valtteri Koskivuori on 18/05/2017.
//  Copyright © 2017-2023 Valtteri Koskivuori. All rights reserved.
//

#pragma once

#include "vector.h"

//Simulated light ray
struct lightRay {
	struct vector start;
	struct vector direction;
};

static inline struct vector alongRay(const struct lightRay *ray, float t) {
	return vec_add(ray->start, vec_scale(ray->direction, t));
}
