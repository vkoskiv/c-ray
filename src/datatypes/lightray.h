//
//  lightRay.h
//  C-ray
//
//  Created by Valtteri Koskivuori on 18/05/2017.
//  Copyright © 2017-2021 Valtteri Koskivuori. All rights reserved.
//

#pragma once

#include "vector.h"

//Simulated light ray
struct lightRay {
	struct vector start;
	struct vector direction;
};

static inline struct lightRay newRay(struct vector start, struct vector direction) {
	return (struct lightRay){start, direction};
}

static inline struct vector alongRay(const struct lightRay *ray, float t) {
	return vecAdd(ray->start, vecScale(ray->direction, t));
}
