//
//  vec3.c
//  C-ray
//
//  Created by Valtteri Koskivuori on 28/02/2015.
//  Copyright © 2015-2019 Valtteri Koskivuori. All rights reserved.
//

#include "../includes.h"
#include "vec3.h"

/* Vector Functions */

vec3 vec3_mul(vec3 v1, vec3 v2)
{
	return (vec3) { v1.x* v2.x, v1.y* v2.y, v1.z* v2.z };
}

vec3 vec3_muls(vec3 v, float x)
{
	return (vec3) { v.x* x, v.y* x, v.z* x };
}

vec3 vec3_add(vec3 v1, vec3 v2)
{
	return (vec3) { v1.x + v2.x, v1.y + v2.y, v1.z + v2.z };
}

float vec3_length(vec3 v)
{
	return sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
}

float vec3_dot(vec3 v1, vec3 v2)
{
	return v1.x * v2.x + v1.y * v2.y + v1.z * v2.z;
}

vec3 vec3_divs(vec3 v, float x)
{
	return (vec3) { v.x / x, v.y / x, v.z / x };
}

vec3 vec3_negate(vec3 v)
{
	return (vec3) { -v.x, -v.y, -v.z };
}

vec3 vec3_normalize(vec3 v)
{
	float l = vec3_length(v);
	return (vec3) { v.x / l, v.y / l, v.z / l };
}

vec3 vec3_sub(vec3 v1, vec3 v2)
{
	return (vec3) { v1.x - v2.x, v1.y - v2.y, v1.z - v2.z };
}

vec3 vec3_cross(vec3 v1, vec3 v2)
{
	return (vec3) {
		((v1.y * v2.z) - (v1.z * v2.y)),
		((v1.z * v2.x) - (v1.x * v2.z)),
		((v1.x * v2.y) - (v1.y * v2.x))
	};
}

vec3 vec3_subs(vec3 v, float x)
{
	return (vec3) { v.x - x, v.y - x, v.z - x };
}

/**
 Create a vector with given position values and return it.

 @param x X component
 @param y Y component
 @param z Z component
 @return Vector with given values
 */
vec3 vecWithPos(float x, float y, float z) {
	return (vec3){x, y, z};
}

vec3 vecZero() {
	return (vec3){0.0, 0.0, 0.0};
}

/**
 Add two vectors and return the resulting vector

 @param v1 Vector 1
 @param v2 Vector 2
 @return Resulting vector
 */
vec3 vecAdd(vec3 v1, vec3 v2) {
	return (vec3){v1.x + v2.x, v1.y + v2.y, v1.z + v2.z};
}

/**
 Compute the length of a vector

 @param v Vector to compute the length for
 @return Length of given vector
 */
float vecLength(vec3 v) {
	return sqrt(v.x*v.x + v.y*v.y + v.z*v.z);
}

float vecLengthSquared(vec3 v) {
	return v.x * v.x + v.y * v.y + v.z * v.z;
}

/**
 Subtract a vector from another and return the resulting vector

 @param v1 Vector to be subtracted from
 @param v2 Vector to be subtracted
 @return Resulting vector
 */
vec3 vecSubtract(const vec3 v1, const vec3 v2) {
	return (vec3){v1.x - v2.x, v1.y - v2.y, v1.z - v2.z};
}

vec3 vecSubtractConst(const vec3 v, float n) {
	return (vec3){v.x - n, v.y - n, v.z - n};
}

/**
 Multiply two vectors and return the 'dot product'

 @param v1 Vector 1
 @param v2 Vector 2
 @return Resulting scalar
 */
float vecDot(const vec3 v1, const vec3 v2) {
	return v1.x * v2.x + v1.y * v2.y + v1.z * v2.z;
}

/**
 Multiply a vector by a given scalar and return the resulting vector

 @param c Scalar to multiply the vector by
 @param v Vector to be multiplied
 @return Multiplied vector
 */
vec3 vecScale(const float c, const vec3 v) {
	return (vec3){v.x * c, v.y * c, v.z * c};
}

vec2 vec2Scale(const float c, const vec2 crd) {
	return (vec2){crd.x * c, crd.y * c};
}

vec2 addvec2s(const vec2 c1, const vec2 c2) {
	return (vec2){c1.x + c2.x, c1.y + c2.y};
}

vec3 vec3_mix(vec3 x0, vec3 x1, float t)
{
	return (vec3)
	{
		(1.0f - t)*x0.x + t*x1.x,
		(1.0f - t)*x0.y + t*x1.y,
		(1.0f - t)*x0.z + t*x1.z
	};
}

/**
 Calculate cross product and return the resulting vector

 @param v1 Vector 1
 @param v2 Vector 2
 @return Cross product of given vectors
 */
vec3 vecCross(vec3 v1, vec3 v2) {
	return (vec3){ ((v1.y * v2.z) - (v1.z * v2.y)),
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
vec3 vecMin(vec3 v1, vec3 v2) {
	return (vec3){min(v1.x, v2.x), min(v1.y, v2.y), min(v1.z, v2.z)};
}

/**
 Return a vector containing the largest components of given vectors

 @param v1 Vector 1
 @param v2 Vector 2
 @return Largest vector
 */
vec3 vecMax(vec3 v1, vec3 v2) {
	return (vec3){max(v1.x, v2.x), max(v1.y, v2.y), max(v1.z, v2.z)};
}


/**
 Normalize a given vector

 @param v Vector to normalize
 @todo Consider having this one void and as a reference type
 @return normalized vector
 */
vec3 vecNormalize(vec3 v) {
	float length = vecLength(v);
	return (vec3){v.x / length, v.y / length, v.z / length};
}


//TODO: Consider just passing polygons to here instead of individual vectors
/**
 Get the mid-point for three given vectors

 @param v1 Vector 1
 @param v2 Vector 2
 @param v3 Vector 3
 @return Mid-point of given vectors
 */
vec3 getMidPoint(vec3 v1, vec3 v2, vec3 v3) {
	return vecScale(1.0/3.0, vecAdd(vecAdd(v1, v2), v3));
}

/**
 Returns a random float between min and max
 
 @param min Minimum value
 @param max Maximum value
 @return Random float between min and max
 */
float rndFloat(float min, float max, pcg32_random_t *rng) {
	return (((float)pcg32_random_r(rng) / (float)UINT32_MAX) * (max - min)) + min;
}

/**
 Returns a randomized position in a radius around a given point
 
 @param center Center point for random distribution
 @param radius Maximum distance from center point
 @return Vector of a random position within a radius of center point
 */
vec3 getRandomVecOnRadius(vec3 center, float radius, pcg32_random_t *rng) {
	return vecWithPos(center.x + rndFloat(-radius, radius, rng),
					  center.y + rndFloat(-radius, radius, rng),
					  center.z + rndFloat(-radius, radius, rng));
}

/**
 Returns a randomized position on a plane in a radius around a given point
 
 @param center Center point for random distribution
 @param radius Maximum distance from center point
 @return Vector of a random position on a plane within a radius of center point
 */
vec3 getRandomVecOnPlane(vec3 center, float radius, pcg32_random_t *rng) {
	//FIXME: This only works in one orientation!
	return vecWithPos(center.x + rndFloat(-radius, radius, rng),
						 center.y + rndFloat(-radius, radius, rng),
						 center.z);
}

vec3 vecMultiply(vec3 v1, vec3 v2) {
	return (vec3){v1.x * v2.x, v1.y * v2.y, v1.z * v2.z};
}

vec3 vecMultiplyConst(vec3 v, float c) {
	return (vec3){v.x * c, v.y * c, v.z * c};
}

vec3 vecNegate(vec3 v) {
	return (vec3){-v.x, -v.y, -v.z};
}

/**
Returns the reflected ray vector from a surface

@param I Incident vector normalized
@param N Normal vector normalized
@return Vector of the reflected ray vector from a surface
*/
vec3 reflect(const vec3 I, const vec3 N) {
	vec3 Imin2dotNI = vecSubtractConst(I, vecDot(N, I) * 2.0);
	return vecMultiply(N, Imin2dotNI);
}
