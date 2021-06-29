//
//  test_vector.h
//  C-ray
//
//  Created by Valtteri on 24.6.2020.
//  Copyright © 2020 Valtteri Koskivuori. All rights reserved.
//

#pragma once

#include "../src/datatypes/vector.h"
#include "../src/renderer/samplers/sampler.h"

#define ATTEMPTS 1024

bool vector_baseWithVec(void) {
	struct vector up = worldUp;
	struct base base = baseWithVec(up);
	vec_roughly_equals(base.i, up);
	struct vector forward = (struct vector){0.0f, 0.0f, -1.0f};
	vec_roughly_equals(base.j, forward);
	struct vector right = (struct vector){-1.0f, 0.0f, 0.0f};
	vec_roughly_equals(base.k, right);
	
	struct vector dots = (struct vector){vecDot(base.i, base.j), vecDot(base.i, base.k), vecDot(base.j, base.k)};
	vec_roughly_equals(dots, vecZero());
	
	// Run random iterations to verify orthogonality
	sampler *sampler = newSampler();
	for (size_t i = 0; i < ATTEMPTS; ++i) {
		initSampler(sampler, Random, (int)i, ATTEMPTS, (int)i * 2);
		struct vector randomDirection = randomOnUnitSphere(sampler);
		struct base randomBase = baseWithVec(randomDirection);
		dots = (struct vector){vecDot(randomBase.i, randomBase.j), vecDot(randomBase.i, randomBase.k), vecDot(randomBase.j, randomBase.k)};
		vec_roughly_equals(dots, vecZero());
	}
	destroySampler(sampler);
	
	return true;
}

bool vector_vecequals(void) {
	struct vector a = (struct vector){1.0f, 1.0f, 1.0f};
	struct vector b = (struct vector){-1.0f, -1.0f, -1.0f};
	test_assert(vecEquals(a, a));
	test_assert(!vecEquals(a, b));
	return true;
}

bool vector_random_on_sphere(void) {
	sampler *sampler = newSampler();
	for (size_t i = 0; i < ATTEMPTS; ++i) {
		initSampler(sampler, Random, (int)i, ATTEMPTS, (int)i * 2);
		const struct vector random = randomOnUnitSphere(sampler);
		const float length = vecLength(random);
		roughly_equals(length, 1.0f);
	}
	destroySampler(sampler);
	return true;
}
