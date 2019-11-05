//
//  vec3.h
//  C-ray
//
//  Created by Valtteri Koskivuori on 28/02/2015.
//  Copyright © 2015-2019 Valtteri Koskivuori. All rights reserved.
//

#pragma once

typedef struct vec3 {
	float x, y, z;
} vec3;

typedef struct ivec2 {
	int x, y;
} ivec2;

vec3 vec3_mix(vec3 x0, vec3 x1, float t);
vec3 vec3_mul(vec3 v1, vec3 v2);
vec3 vec3_muls(vec3 v, float x);
vec3 vec3_divs(vec3 v, float x);
vec3 vec3_add(vec3 v1, vec3 v2);
vec3 vec3_muls(vec3 v, float x);
vec3 vec3_negate(vec3 v);
vec3 vec3_normalize(vec3 v);
vec3 vec3_sub(vec3 v1, vec3 v2);
vec3 vec3_cross(vec3 v1, vec3 v2);
vec3 vec3_subs(vec3 v, float x);
vec3 vec3_adds(vec3 v, float x);

float vec3_length(vec3 v);
float vec3_dot(vec3 v1, vec3 v2);

//Return a vector with given coordinates
vec3 vecWithPos(float x, float y, float z);

//For defaults
vec3 vecZero(void);

//Add two vectors and return the resulting vector
vec3 vecAdd(vec3 v1, vec3 v2);

//Subtract two vectors and return the resulting vector
vec3 vecSubtract(const vec3 v1, const vec3 v2);

//Multiply two vectors and return the dot product
float vecDot(const vec3 v1, const vec3 v2);

//Multiply a vector by a coefficient and return the resulting vector
vec3 vecScale(const float c, const vec3 v);

//Calculate the cross product of two vectors and return the resulting vector
vec3 vecCross(vec3 v1, vec3 v2);

//Calculate min of 2 vectors
vec3 vecMin(vec3 v1, vec3 v2);

//Calculate max of 2 vectors
vec3 vecMax(vec3 v1, vec3 v2);

//Calculate length of vector
float vecLength(vec3 v);

//calculate length^2 of vector
float vecLengthSquared(vec3 v);

//Normalize a vector
vec3 vecNormalize(vec3 v);

vec3 getMidPoint(vec3 v1, vec3 v2, vec3 v3);

vec3 getRandomVecOnRadius(vec3 center, float radius, pcg32_random_t *rng);

vec3 getRandomVecOnPlane(vec3 center, float radius, pcg32_random_t *rng);

float rndFloatRange(float min, float max, pcg32_random_t *rng);
float randomFloat(pcg32_random_t *rng);

vec3 vecMultiplyConst(vec3 v, const float c);

vec3 vecMultiply(vec3 v1, vec3 v2);

vec3 reflect(const vec3 I, const vec3 N);

vec3 vecNegate(vec3 v);
