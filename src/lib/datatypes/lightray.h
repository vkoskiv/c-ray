//
//  lightRay.h
//  c-ray
//
//  Created by Valtteri Koskivuori on 18/05/2017.
//  Copyright © 2017-2024 Valtteri Koskivuori. All rights reserved.
//

#pragma once

#include "../../common/vector.h"
#include "../../common/transforms.h"

enum ray_type {
	rt_camera       = 1 << 1,
	rt_shadow       = 1 << 2,  // TODO
	rt_diffuse      = 1 << 3,
	rt_glossy       = 1 << 4,
	rt_singular     = 1 << 5, // TODO
	rt_reflection   = 1 << 6,
	rt_transmission = 1 << 7,
};

//Simulated light ray
struct lightRay {
	struct vector start;
	struct vector direction;
	enum ray_type type : 8;
};

static inline struct vector alongRay(const struct lightRay *ray, float t) {
	return vec_add(ray->start, vec_scale(ray->direction, t));
}

static inline void tform_ray(struct lightRay *ray, const struct matrix4x4 mat) {
	tform_point(&ray->start, mat);
	tform_vector(&ray->direction, mat);
}
