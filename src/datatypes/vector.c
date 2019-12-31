//
//  vector.c
//  C-ray
//
//  Created by Valtteri Koskivuori on 28/02/2015.
//  Copyright © 2015-2020 Valtteri Koskivuori. All rights reserved.
//

#include "../includes.h"
#include "vector.h"

/* Vector Functions */

/**
 Create a vector with given position values and return it.

 @param x X component
 @param y Y component
 @param z Z component
 @return Vector with given values
 */
struct vector vecWithPos(float x, float y, float z) {
	return (struct vector){x, y, z};
}

struct vector vecZero() {
	return (struct vector){0.0f, 0.0f, 0.0f};
}

/**
 Add two vectors and return the resulting vector

 @param v1 Vector 1
 @param v2 Vector 2
 @return Resulting vector
 */
struct vector vecAdd(struct vector v1, struct vector v2) {
	return (struct vector){v1.x + v2.x, v1.y + v2.y, v1.z + v2.z};
}

/**
 Subtract a vector from another and return the resulting vector

 @param v1 Vector to be subtracted from
 @param v2 Vector to be subtracted
 @return Resulting vector
 */
struct vector vecSub(const struct vector v1, const struct vector v2) {
	return (struct vector){v1.x - v2.x, v1.y - v2.y, v1.z - v2.z};
}

struct vector vecMul(struct vector v1, struct vector v2) {
	return (struct vector){v1.x * v2.x, v1.y * v2.y, v1.z * v2.z};
}

/**
 Compute the length of a vector

 @param v Vector to compute the length for
 @return Length of given vector
 */
float vecLength(struct vector v) {
	return sqrt(v.x*v.x + v.y*v.y + v.z*v.z);
}

float vecLengthSquared(struct vector v) {
	return v.x * v.x + v.y * v.y + v.z * v.z;
}

struct vector vecSubtractConst(const struct vector v, float n) {
	return (struct vector){v.x - n, v.y - n, v.z - n};
}

/**
 Multiply two vectors and return the 'dot product'

 @param v1 Vector 1
 @param v2 Vector 2
 @return Resulting scalar
 */
float vecDot(const struct vector v1, const struct vector v2) {
	return v1.x * v2.x + v1.y * v2.y + v1.z * v2.z;
}

/**
 Multiply a vector by a given scalar and return the resulting vector

 @param c Scalar to multiply the vector by
 @param v Vector to be multiplied
 @return Multiplied vector
 */
struct vector vecScale(const struct vector v, const float c) {
	return (struct vector){v.x * c, v.y * c, v.z * c};
}

struct coord coordScale(const float c, const struct coord crd) {
	return (struct coord){crd.x * c, crd.y * c};
}

struct coord addCoords(const struct coord c1, const struct coord c2) {
	return (struct coord){c1.x + c2.x, c1.y + c2.y};
}

/**
 Calculate cross product and return the resulting vector

 @param v1 Vector 1
 @param v2 Vector 2
 @return Cross product of given vectors
 */
struct vector vecCross(struct vector v1, struct vector v2) {
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
struct vector vecMin(struct vector v1, struct vector v2) {
	return (struct vector){min(v1.x, v2.x), min(v1.y, v2.y), min(v1.z, v2.z)};
}

/**
 Return a vector containing the largest components of given vectors

 @param v1 Vector 1
 @param v2 Vector 2
 @return Largest vector
 */
struct vector vecMax(struct vector v1, struct vector v2) {
	return (struct vector){max(v1.x, v2.x), max(v1.y, v2.y), max(v1.z, v2.z)};
}


/**
 Normalize a given vector

 @param v Vector to normalize
 @todo Consider having this one void and as a reference type
 @return normalized vector
 */
struct vector vecNormalize(struct vector v) {
	float length = vecLength(v);
	return (struct vector){v.x / length, v.y / length, v.z / length};
}


//TODO: Consider just passing polygons to here instead of individual vectors
/**
 Get the mid-point for three given vectors

 @param v1 Vector 1
 @param v2 Vector 2
 @param v3 Vector 3
 @return Mid-point of given vectors
 */
struct vector getMidPoint(struct vector v1, struct vector v2, struct vector v3) {
	return vecScale(vecAdd(vecAdd(v1, v2), v3), 1.0f/3.0f);
}

/**
 Returns a random float between min and max
 
 @param min Minimum value
 @param max Maximum value
 @return Random float between min and max
 */
float rndFloatRange(float min, float max, pcg32_random_t *rng) {
	return (((float)pcg32_random_r(rng) / (float)UINT32_MAX) * (max - min)) + min;
}

float rndFloat(pcg32_random_t *rng) {
	return (1.0f / (1ull << 32)) * pcg32_random_r(rng);
}

/**
 Returns a randomized position in a radius around a given point
 
 @param center Center point for random distribution
 @param radius Maximum distance from center point
 @return Vector of a random position within a radius of center point
 */
struct vector getRandomVecOnRadius(struct vector center, float radius, pcg32_random_t *rng) {
	return vecWithPos(center.x + rndFloatRange(-radius, radius, rng),
					  center.y + rndFloatRange(-radius, radius, rng),
					  center.z + rndFloatRange(-radius, radius, rng));
}

/**
 Returns a randomized position on a plane in a radius around a given point
 
 @param center Center point for random distribution
 @param radius Maximum distance from center point
 @return Vector of a random position on a plane within a radius of center point
 */
struct vector getRandomVecOnPlane(struct vector center, float radius, pcg32_random_t *rng) {
	//FIXME: This only works in one orientation!
	return vecWithPos(center.x + rndFloatRange(-radius, radius, rng),
						 center.y + rndFloatRange(-radius, radius, rng),
						 center.z);
}

struct vector vecNegate(struct vector v) {
	return (struct vector){-v.x, -v.y, -v.z};
}

struct vector vecReflect(const struct vector I, const struct vector N) {
	return vecSub(I, vecScale(N, vecDot(N, I) * 2.0f));
}
