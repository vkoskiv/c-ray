//
//  test_vector.h
//  C-ray
//
//  Created by Valtteri on 24.6.2020.
//  Copyright © 2020-2021 Valtteri Koskivuori. All rights reserved.
//

#pragma once

#include "../src/common/vector.h"
#include "../src/common/transforms.h"
#include "../src/lib/renderer/samplers/sampler.h"
#include "../src/lib/renderer/samplers/vec.h"

#define ATTEMPTS 1024

bool vector_vecZero(void) {
	struct vector a = vec_zero();
	struct vector expected = (struct vector){0.0f, 0.0f, 0.0f};
	vec_roughly_equals(a, expected);
	return true;
}

bool vector_vecAdd(void) {
	struct vector a = (struct vector){1.0f, 2.0f, 3.0f};
	struct vector b = (struct vector){4.0f, 5.0f, 6.0f};
	struct vector added = vec_add(a, b);
	struct vector expected = (struct vector){5.0f, 7.0f, 9.0f};
	vec_roughly_equals(added, expected);
	return true;
}

bool vector_vecSub(void) {
	struct vector a = (struct vector){1.0f, 2.0f, 3.0f};
	struct vector b = (struct vector){4.0f, 5.0f, 6.0f};
	struct vector subtracted = vec_sub(a, b);
	struct vector expected = (struct vector){-3.0f, -3.0f, -3.0f};
	vec_roughly_equals(subtracted, expected);
	return true;
}

bool vector_vecMul(void) {
	struct vector a = (struct vector){1.0f, 2.0f, 3.0f};
	struct vector b = (struct vector){4.0f, 5.0f, 6.0f};
	struct vector multiplied = vec_mul(a, b);
	struct vector expected = (struct vector){4.0f, 10.0f, 18.0f};
	vec_roughly_equals(multiplied, expected);
	return true;
}

bool vector_dot(void) {
	struct vector up = g_world_up;
	struct vector right = (struct vector){1.0f, 0.0f, 0.0f};
	// These should be perpendicular, i.e. dot product should be 0
	float dot = vec_dot(up, right);
	roughly_equals(dot, 0.0f);
	
	struct vector down = vec_negate(g_world_up);
	dot = vec_dot(up, down);
	roughly_equals(dot, -1.0f);
	
	dot = vec_dot(up, up);
	roughly_equals(dot, 1.0f);
	return true;
}

bool vector_vecScale(void) {
	struct vector to_be_scaled = (struct vector){0.0f, 1.0f, 0.0f};
	struct vector scaled = vec_scale(to_be_scaled, 2.0f);
	struct vector expected_scaled = (struct vector){0.0f, 2.0f, 0.0f};
	vec_roughly_equals(scaled, expected_scaled);
	return true;
}

bool vector_vecCross(void) {
	struct vector a = (struct vector){1.0f, 0.0f, 0.0f};
	struct vector b = (struct vector){0.0f, 1.0f, 0.0f};
	struct vector cross = vec_cross(a, b);
	struct vector expected = (struct vector){0.0f, 0.0f, 1.0f};
	vec_roughly_equals(cross, expected);
	return true;
}

bool vector_vecMin(void) {
	struct vector smaller = (struct vector){1.0f, 1.0f, 1.0f};
	struct vector larger = (struct vector){10.0f, 10.0f, 10.0f};
	struct vector min = vec_min(smaller, larger);
	struct vector expected = smaller;
	vec_roughly_equals(min, expected);
	return true;
}

bool vector_vecMax(void) {
	struct vector smaller = (struct vector){1.0f, 1.0f, 1.0f};
	struct vector larger = (struct vector){10.0f, 10.0f, 10.0f};
	struct vector max = vec_max(smaller, larger);
	struct vector expected = larger;
	vec_roughly_equals(max, expected);
	return true;
}

bool vector_vecLengthSquared(void) {
	struct vector a = (struct vector){3.0f, 0.0f, 0.0f};
	roughly_equals(vec_length_squared(a), 9.0f);
	return true;
}

bool vector_vecLength(void) {
	struct vector a = (struct vector){3.0f, 0.0f, 0.0f};
	roughly_equals(vec_length(a), 3.0f);
	return true;
}

bool vector_vecNormalize(void) {
	struct vector a = vec_normalize((struct vector){123.0f, -345.0f, 789.0f});
	roughly_equals(vec_length(a), 1.0f);
	return true;
}

bool vector_getMidPoint(void) {
	struct transform rotate120 = tform_new_rot_z(deg_to_rad(120.0f));
	struct vector a = vec_normalize((struct vector){0.0f, 1.0f, 0.0f});
	struct vector b = a;
	tform_vector(&b, rotate120.A);
	struct vector c = b;
	tform_vector(&c, rotate120.A);
	struct vector mid = vec_get_midpoint(a, b, c);
	vec_roughly_equals(mid, vec_zero());
	return true;
}

bool vector_vecNegate(void) {
	struct vector a = (struct vector){1.0f, 1.0f, 1.0f};
	struct vector ainv = vec_negate(a);
	struct vector expected = (struct vector){-1.0f, -1.0f, -1.0f};
	vec_roughly_equals(ainv, expected);
	return true;
}

bool vector_baseWithVec(void) {
	struct vector up = g_world_up;
	struct base base = baseWithVec(up);
	vec_roughly_equals(base.i, up);
	struct vector forward = (struct vector){0.0f, 0.0f, -1.0f};
	vec_roughly_equals(base.j, forward);
	struct vector right = (struct vector){-1.0f, 0.0f, 0.0f};
	vec_roughly_equals(base.k, right);
	
	struct vector dots = (struct vector){vec_dot(base.i, base.j), vec_dot(base.i, base.k), vec_dot(base.j, base.k)};
	vec_roughly_equals(dots, vec_zero());
	
	// Run random iterations to verify orthogonality
	sampler *sampler = sampler_new();
	for (size_t i = 0; i < ATTEMPTS; ++i) {
		sampler_init(sampler, Random, (int)i, ATTEMPTS, (int)i * 2);
		struct vector randomDirection = vec_on_unit_sphere(sampler);
		struct base randomBase = baseWithVec(randomDirection);
		dots = (struct vector){vec_dot(randomBase.i, randomBase.j), vec_dot(randomBase.i, randomBase.k), vec_dot(randomBase.j, randomBase.k)};
		vec_roughly_equals(dots, vec_zero());
	}
	sampler_destroy(sampler);
	
	return true;
}

bool vector_vecequals(void) {
	struct vector a = (struct vector){1.0f, 1.0f, 1.0f};
	struct vector b = (struct vector){-1.0f, -1.0f, -1.0f};
	test_assert(vec_equals(a, a));
	test_assert(!vec_equals(a, b));
	return true;
}

bool vector_random_on_sphere(void) {
	sampler *sampler = sampler_new();
	for (size_t i = 0; i < ATTEMPTS; ++i) {
		sampler_init(sampler, Random, (int)i, ATTEMPTS, (int)i * 2);
		const struct vector random = vec_on_unit_sphere(sampler);
		const float length = vec_length(random);
		roughly_equals(length, 1.0f);
	}
	sampler_destroy(sampler);
	return true;
}

bool vector_reflect(void) {
	struct vector toReflect = vec_normalize((struct vector){1.0f, 1.0f, 0.0f});
	struct vector normal = (struct vector){0.0f, -1.0f, 0.0f};
	struct vector reflected = vec_reflect(toReflect, normal);
	float reflected_length = vec_length(reflected);
	roughly_equals(reflected_length, 1.0f);
	struct vector expected = vec_normalize((struct vector){1.0f, -1.0f, 0.0f});
	vec_roughly_equals(reflected, expected);
	// In this specific test case, the ray entered at 45°, therefore the reflected ray
	// should be orthogonal to the incident one. This doesn't apply in most cases.
	float dot = vec_dot(toReflect, reflected);
	roughly_equals(dot, 0.0f);
	return true;
}
