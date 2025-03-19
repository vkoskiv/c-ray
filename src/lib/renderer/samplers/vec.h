//
//  vec.h
//  c-ray
//
//  Created by Valtteri on 02.12.2023.
//  Copyright © 2023-2025 Valtteri Koskivuori. All rights reserved.
//

#include "sampler.h"
#include <common/vector.h>

// FIXME: Move to a better place
static inline float rand_in_range(float min, float max, sampler *sampler) {
	return ((sampler_dimension(sampler)) * (max - min)) + min;
}

static inline struct coord coord_on_unit_disc(sampler *sampler) {
	float r = sqrtf(sampler_dimension(sampler));
	float theta = rand_in_range(0.0f, 2.0f * PI, sampler);
	return (struct coord){r * cosf(theta), r * sinf(theta)};
}

static inline struct vector vec_on_unit_sphere(sampler *sampler) {
	const float sample_x = sampler_dimension(sampler);
	const float sample_y = sampler_dimension(sampler);
	const float a = sample_x * (2.0f * PI);
	const float s = 2.0f * sqrtf(max(0.0f, sample_y * (1.0f - sample_y)));
	return (struct vector){ cosf(a) * s, sinf(a) * s, 1.0f - 2.0f * sample_y };
}

