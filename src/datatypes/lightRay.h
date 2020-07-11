//
//  lightRay.h
//  C-ray
//
//  Created by Valtteri Koskivuori on 18/05/2017.
//  Copyright © 2015-2020 Valtteri Koskivuori. All rights reserved.
//

#pragma once

#include "vector.h"

enum type {
	rayTypeIncident,
	rayTypeScattered,
	rayTypeReflected,
	rayTypeRefracted
};

//Simulated light ray
struct lightRay {
	struct vector start;
	struct vector direction;
	enum type rayType;
};

static inline struct lightRay newRay(struct vector start, struct vector direction, enum type rayType) {
	return (struct lightRay){start, direction, rayType};
}

static inline struct vector alongRay(const struct lightRay ray, float t) {
	return vecAdd(ray.start, vecScale(ray.direction, t));
}
