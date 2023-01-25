//
//  vector.h
//  C-ray
//
//  Created by Valtteri Koskivuori on 28/02/2015.
//  Copyright © 2015-2021 Valtteri Koskivuori. All rights reserved.
//

#pragma once

#include <math.h>
#include "../renderer/samplers/sampler.h"
#include "../utils/assert.h"
#include "../includes.h"

struct vector {
	float x, y, z;
};

struct base {
	struct vector i, j, k;
};

struct coord {
	float x, y;
};

struct intCoord {
	int x, y;
};

static const struct vector g_world_up = { 0.0f, 1.0f, 0.0f };

//For defaults
static inline struct vector vec_zero() {
	return (struct vector){ 0.0f, 0.0f, 0.0f };
}

static inline struct coord coord_zero() {
	return (struct coord){ 0.0f, 0.0f };
}

static inline float clamp(float value, float min, float max) {
	return min(max(value, min), max);
}

static inline float vec_component(const struct vector *v, unsigned axis) {
	return (&v->x)[axis];
}

/**
 Add two vectors and return the resulting vector

 @param v1 Vector 1
 @param v2 Vector 2
 @return Resulting vector
 */
static inline struct vector vec_add(struct vector v1, struct vector v2) {
	return (struct vector){ v1.x + v2.x, v1.y + v2.y, v1.z + v2.z };
}

/**
 Subtract a vector from another and return the resulting vector

 @param v1 Vector to be subtracted from
 @param v2 Vector to be subtracted
 @return Resulting vector
 */
static inline struct vector vec_sub(const struct vector v1, const struct vector v2) {
	return (struct vector){ v1.x - v2.x, v1.y - v2.y, v1.z - v2.z };
}

static inline struct vector vec_mul(struct vector v1, struct vector v2) {
	return (struct vector){ v1.x * v2.x, v1.y * v2.y, v1.z * v2.z };
}

/**
 Return the dot product of two vectors

 @param v1 Vector 1
 @param v2 Vector 2
 @return Resulting scalar
 */
static inline float vec_dot(const struct vector v1, const struct vector v2) {
	return v1.x * v2.x + v1.y * v2.y + v1.z * v2.z;
}

/**
 Multiply a vector by a given scalar and return the resulting vector

 @param c Scalar to multiply the vector by
 @param v Vector to be multiplied
 @return Multiplied vector
 */
static inline struct vector vec_scale(const struct vector v, const float c) {
	return (struct vector){ v.x * c, v.y * c, v.z * c };
}

static inline struct coord coord_scale(const float c, const struct coord crd) {
	return (struct coord){ crd.x * c, crd.y * c };
}

static inline struct coord coord_add(const struct coord c1, const struct coord c2) {
	return (struct coord){ c1.x + c2.x, c1.y + c2.y };
}

/**
 Calculate cross product and return the resulting vector

 @param v1 Vector 1
 @param v2 Vector 2
 @return Cross product of given vectors
 */
static inline struct vector vec_cross(struct vector v1, struct vector v2) {
	return (struct vector){ ((v1.y * v2.z) - (v1.z * v2.y)),
							((v1.z * v2.x) - (v1.x * v2.z)),
							((v1.x * v2.y) - (v1.y * v2.x))
	};
}

/**
 Return a vector containing the smallest components of given vectors

 @param v1 Vector 1
 @param v2 Vector 2
 @return Smallest vector
 */
static inline struct vector vec_min(struct vector v1, struct vector v2) {
	return (struct vector){ min(v1.x, v2.x), min(v1.y, v2.y), min(v1.z, v2.z) };
}

/**
 Return a vector containing the largest components of given vectors

 @param v1 Vector 1
 @param v2 Vector 2
 @return Largest vector
 */
static inline struct vector vec_max(struct vector v1, struct vector v2) {
	return (struct vector){ max(v1.x, v2.x), max(v1.y, v2.y), max(v1.z, v2.z) };
}

//calculate length^2 of vector
static inline float vec_length_squared(struct vector v) {
	return vec_dot(v, v);
}

/**
 Compute the length of a vector

 @param v Vector to compute the length for
 @return Length of given vector
 */
static inline float vec_length(struct vector v) {
	return sqrtf(vec_length_squared(v));
}

/**
 Normalize a given vector

 @param v Vector to normalize
 @todo Consider having this one void and as a reference type
 @return normalized vector
 */
static inline struct vector vec_normalize(struct vector v) {
	float length = vec_length(v);
	return (struct vector){ v.x / length, v.y / length, v.z / length };
}

/**
 Get the mid-point for three given vectors

 @param v1 Vector 1
 @param v2 Vector 2
 @param v3 Vector 3
 @return Mid-point of given vectors
 */
static inline struct vector vec_get_midpoint(struct vector v1, struct vector v2, struct vector v3) {
	return vec_scale(vec_add(vec_add(v1, v2), v3), 1.0f/3.0f);
}

static inline float rand_in_range(float min, float max, sampler *sampler) {
	return ((getDimension(sampler)) * (max - min)) + min;
}

static inline struct coord coord_on_unit_disc(sampler *sampler) {
	float r = sqrtf(getDimension(sampler));
	float theta = rand_in_range(0.0f, 2.0f * PI, sampler);
	return (struct coord){r * cosf(theta), r * sinf(theta)};
}

static inline struct vector vec_negate(struct vector v) {
	return (struct vector){ -v.x, -v.y, -v.z };
}

/**
Returns the reflected ray vector from a surface

@param I Incident vector normalized
@param N Normal vector normalized
@return Vector of the reflected ray vector from a surface
*/
static inline struct vector vec_reflect(const struct vector I, const struct vector N) {
	return vec_sub(I, vec_scale(N, vec_dot(N, I) * 2.0f));
}

static inline float vec_distance_to(const struct vector a, const struct vector b) {
	return sqrtf(powf((b.x - a.x), 2) + powf((b.y - a.y), 2) + powf((b.z - a.z), 2));
}

static inline float wrap_max(float x, float max) {
	return fmodf(max + fmodf(x, max), max);
}

static inline float wrap_min_max(float x, float min, float max) {
	return min + wrap_max(x - min, max - min);
}

//Compute two orthonormal vectors for this unit vector
//PBRT
static inline struct base baseWithVec(struct vector i) {
	struct base newBase;
	newBase.i = i;
	if (fabsf(i.x) > fabsf(i.y)) {
		float len = sqrtf(i.x * i.x + i.z * i.z);
		newBase.j = (struct vector){-i.z / len, 0.0f / len, i.x / len};
	} else {
		float len = sqrtf(i.y * i.y + i.z * i.z);
		newBase.j = (struct vector){ 0.0f / len, i.z / len, -i.y / len};
	}
	newBase.k = vec_cross(newBase.i, newBase.j);
	return newBase;
}

static inline bool vec_equals(struct vector a, struct vector b) {
	return a.x == b.x && a.y == b.y && a.z == b.z;
}

static inline struct vector vec_on_unit_sphere(sampler *sampler) {
	const float sample_x = getDimension(sampler);
	const float sample_y = getDimension(sampler);
	const float a = sample_x * (2.0f * PI);
	const float s = 2.0f * sqrtf(max(0.0f, sample_y * (1.0f - sample_y)));
	return (struct vector){ cosf(a) * s, sinf(a) * s, 1.0f - 2.0f * sample_y };
}

//TODO: Consider moving these two to a better place.
static inline bool vec_refract(const struct vector in, const struct vector normal, float niOverNt, struct vector *refracted) {
	const struct vector uv = vec_normalize(in);
	const float dt = vec_dot(uv, normal);
	const float discriminant = 1.0f - niOverNt * niOverNt * (1.0f - dt * dt);
	if (discriminant > 0.0f) {
		const struct vector A = vec_scale(normal, dt);
		const struct vector B = vec_sub(uv, A);
		const struct vector C = vec_scale(B, niOverNt);
		const struct vector D = vec_scale(normal, sqrtf(discriminant));
		*refracted = vec_sub(C, D);
		return true;
	} else {
		return false;
	}
}

static inline float schlick(float cosine, float IOR) {
	float r0 = (1.0f - IOR) / (1.0f + IOR);
	r0 = r0 * r0;
	return r0 + (1.0f - r0) * powf((1.0f - cosine), 5.0f);
}
