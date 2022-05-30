//
//  test_vector.h
//  C-ray
//
//  Created by Valtteri on 24.6.2020.
//  Copyright © 2020-2021 Valtteri Koskivuori. All rights reserved.
//

#pragma once

#include "../src/datatypes/vector.h"
#include "../src/renderer/samplers/sampler.h"

#define ATTEMPTS 1024

bool vector_vecZero(void) {
	struct vector a = vecZero();
	struct vector expected = (struct vector){0.0f, 0.0f, 0.0f};
	vec_roughly_equals(a, expected);
	return true;
}

bool vector_vecAdd(void) {
	struct vector a = (struct vector){1.0f, 2.0f, 3.0f};
	struct vector b = (struct vector){4.0f, 5.0f, 6.0f};
	struct vector added = vecAdd(a, b);
	struct vector expected = (struct vector){5.0f, 7.0f, 9.0f};
	vec_roughly_equals(added, expected);
	return true;
}

bool vector_vecSub(void) {
	struct vector a = (struct vector){1.0f, 2.0f, 3.0f};
	struct vector b = (struct vector){4.0f, 5.0f, 6.0f};
	struct vector subtracted = vecSub(a, b);
	struct vector expected = (struct vector){-3.0f, -3.0f, -3.0f};
	vec_roughly_equals(subtracted, expected);
	return true;
}

bool vector_vecMul(void) {
	struct vector a = (struct vector){1.0f, 2.0f, 3.0f};
	struct vector b = (struct vector){4.0f, 5.0f, 6.0f};
	struct vector multiplied = vecMul(a, b);
	struct vector expected = (struct vector){4.0f, 10.0f, 18.0f};
	vec_roughly_equals(multiplied, expected);
	return true;
}

bool vector_dot(void) {
	struct vector up = worldUp;
	struct vector right = (struct vector){1.0f, 0.0f, 0.0f};
	// These should be perpendicular, i.e. dot product should be 0
	float dot = vecDot(up, right);
	roughly_equals(dot, 0.0f);
	
	struct vector down = vecNegate(worldUp);
	dot = vecDot(up, down);
	roughly_equals(dot, -1.0f);
	
	dot = vecDot(up, up);
	roughly_equals(dot, 1.0f);
	return true;
}

bool vector_vecScale(void) {
	struct vector to_be_scaled = (struct vector){0.0f, 1.0f, 0.0f};
	struct vector scaled = vecScale(to_be_scaled, 2.0f);
	struct vector expected_scaled = (struct vector){0.0f, 2.0f, 0.0f};
	vec_roughly_equals(scaled, expected_scaled);
	return true;
}

bool vector_vecCross(void) {
	struct vector a = (struct vector){1.0f, 0.0f, 0.0f};
	struct vector b = (struct vector){0.0f, 1.0f, 0.0f};
	struct vector cross = vecCross(a, b);
	struct vector expected = (struct vector){0.0f, 0.0f, 1.0f};
	vec_roughly_equals(cross, expected);
	return true;
}

bool vector_vecMin(void) {
	struct vector smaller = (struct vector){1.0f, 1.0f, 1.0f};
	struct vector larger = (struct vector){10.0f, 10.0f, 10.0f};
	struct vector max = vecMax(smaller, larger);
	struct vector expectedMax = larger;
	vec_roughly_equals(max, expectedMax);
	return true;
}

bool vector_vecMax(void) {
	struct vector smaller = (struct vector){1.0f, 1.0f, 1.0f};
	struct vector larger = (struct vector){10.0f, 10.0f, 10.0f};
	struct vector max = vecMax(smaller, larger);
	struct vector expectedMax = larger;
	vec_roughly_equals(max, expectedMax);
	return true;
}

bool vector_vecLengthSquared(void) {
	struct vector a = (struct vector){3.0f, 0.0f, 0.0f};
	roughly_equals(vecLengthSquared(a), 9.0f);
	return true;
}

bool vector_vecLength(void) {
	struct vector a = (struct vector){3.0f, 0.0f, 0.0f};
	roughly_equals(vecLength(a), 3.0f);
	return true;
}

bool vector_vecNormalize(void) {
	struct vector a = vecNormalize((struct vector){123.0f, -345.0f, 789.0f});
	roughly_equals(vecLength(a), 1.0f);
	return true;
}

bool vector_getMidPoint(void) {
	struct transform rotate120 = newTransformRotateZ(toRadians(120.0f));
	struct vector a = vecNormalize((struct vector){0.0f, 1.0f, 0.0f});
	struct vector b = a;
	transformVector(&b, rotate120.A);
	struct vector c = b;
	transformVector(&c, rotate120.A);
	struct vector mid = getMidPoint(a, b, c);
	vec_roughly_equals(mid, vecZero());
	return true;
}

bool vector_vecNegate(void) {
	struct vector a = (struct vector){1.0f, 1.0f, 1.0f};
	struct vector ainv = vecNegate(a);
	struct vector expected = (struct vector){-1.0f, -1.0f, -1.0f};
	vec_roughly_equals(ainv, expected);
	return true;
}

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

bool vector_reflect(void) {
	struct vector toReflect = vecNormalize((struct vector){1.0f, 1.0f, 0.0f});
	struct vector normal = (struct vector){0.0f, -1.0f, 0.0f};
	struct vector reflected = vecReflect(toReflect, normal);
	float reflected_length = vecLength(reflected);
	roughly_equals(reflected_length, 1.0f);
	struct vector expected = vecNormalize((struct vector){1.0f, -1.0f, 0.0f});
	vec_roughly_equals(reflected, expected);
	// In this specific test case, the ray entered at 45°, therefore the reflected ray
	// should be orthogonal to the incident one. This doesn't apply in most cases.
	float dot = vecDot(toReflect, reflected);
	roughly_equals(dot, 0.0f);
	return true;
}
