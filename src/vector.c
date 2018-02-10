//
//  vector.c
//  C-Ray
//
//  Created by Valtteri Koskivuori on 28/02/15.
//  Copyright (c) 2015 Valtteri Koskivuori. All rights reserved.
//

#include "includes.h"
#include "vector.h"

//Main vertex arrays

/*
 Note:
 C-Ray stores all vectors and polygons in shared arrays, and uses data structures
 to keep track of them.
 */

struct vector *vertexArray;
int vertexCount;
struct vector *normalArray;
int normalCount;
struct vector *textureArray;
int textureCount;

/* Vector Functions */

/**
 Create a vector with given position values and return it.

 @param x X component
 @param y Y component
 @param z Z component
 @return Vector with given values
 */
struct vector vectorWithPos(double x, double y, double z) {
	return (struct vector){x, y, z, false};
}

/**
 Add two vectors and return the resulting vector

 @param v1 Vector 1
 @param v2 Vector 2
 @return Resulting vector
 */
struct vector addVectors(struct vector *v1, struct vector *v2) {
	return (struct vector){v1->x + v2->x, v1->y + v2->y, v1->z + v2->z, false};
}

/**
 Compute the length of a vector

 @param v Vector to compute the length for
 @return Length of given vector
 */
double vectorLength(struct vector *v) {
	return sqrt(v->x*v->x + v->y*v->y + v->z*v->z);
}

/**
 Subtract a vector from another and return the resulting vector

 @param v1 Vector to be subtracted from
 @param v2 Vector to be subtracted
 @return Resulting vector
 */
struct vector subtractVectors(const struct vector *v1, const struct vector *v2) {
	return (struct vector){v1->x - v2->x, v1->y - v2->y, v1->z - v2->z, false};
}

struct vector vectorSubtract(const struct vector* v, double n) {
	struct vector rv = {v->x - n, v->y - n, v->z - n, false};
	return rv;
}

/**
 Multiply two vectors and return the 'dot product'

 @param v1 Vector 1
 @param v2 Vector 2
 @return Resulting vector
 */
double scalarProduct(const struct vector *v1, const struct vector *v2) {
	return v1->x * v2->x + v1->y * v2->y + v1->z * v2->z;
}

/**
 Multiply a vector by a given scalar and return the resulting vector

 @param c Scalar to multiply the vector by
 @param v Vector to be multiplied
 @return Multiplied vector
 */
struct vector vectorScale(const double c, const struct vector *v) {
	return (struct vector){v->x * c, v->y * c, v->z * c, false};
}

/**
 Calculate cross product and return the resulting vector

 @param v1 Vector 1
 @param v2 Vector 2
 @return Cross product of given vectors
 */
struct vector vectorCross(struct vector *v1, struct vector *v2) {
	return (struct vector){((v1->y * v2->z) - (v1->z * v2->y)),
		((v1->z * v2->x) - (v1->x * v2->z)),
		((v1->x * v2->y) - (v1->y * v2->x)), false};
}

/**
 Return a vector containing the smallest components of given vectors

 @param v1 Vector 1
 @param v2 Vector 2
 @return Smallest vector
 */
struct vector minVector(struct vector *v1, struct vector *v2) {
	return (struct vector){min(v1->x, v2->x), min(v1->y, v2->y), min(v1->z, v2->z), false};
}

/**
 Return a vector containing the largest components of given vectors

 @param v1 Vector 1
 @param v2 Vector 2
 @return Largest vector
 */
struct vector maxVector(struct vector *v1, struct vector *v2) {
	return (struct vector){max(v1->x, v2->x), max(v1->y, v2->y), max(v1->z, v2->z), false};
}


/**
 Normalize a given vector

 @param v Vector to normalize
 @return normalized vector
 */
struct vector normalizeVector(struct vector *v) {
	double length = vectorLength(v);
	return (struct vector){v->x / length, v->y / length, v->z / length, false};
}


//TODO: Consider just passing polygons to here instead of individual vectors
/**
 Get the mid-point for three given vectors

 @param v1 Vector 1
 @param v2 Vector 2
 @param v3 Vector 3
 @return Mid-point of given vectors
 */
struct vector getMidPoint(struct vector *v1, struct vector *v2, struct vector *v3) {
	struct vector temp = addVectors(v1, v2);
	temp = addVectors(&temp, v3);
	return vectorScale(1.0/3.0, &temp);
}

/**
 Construct and return a UV coordinate from given values

 @param u U component
 @param v V component
 @return coordinate object
 */
struct coord uvFromValues(double u, double v) {
	return (struct coord){u, v};
}

/**
 Returns a random double between min and max
 
 @param min Minimum value
 @param max Maximum value
 @return Random double between min and max
 */
double getRandomDouble(double min, double max) {
	return ((((double)rand()) / (double)RAND_MAX) * (max - min)) + min;
}

/**
 Returns a randomized position in a radius around a given point
 
 @param center Center point for random distribution
 @param radius Maximum distance from center point
 @return Vector of a random position within a radius of center point
 */
struct vector getRandomVecOnRadius(struct vector center, double radius) {
	return vectorWithPos(center.x + getRandomDouble(-radius, radius),
						 center.y + getRandomDouble(-radius, radius),
						 center.z + getRandomDouble(-radius, radius));
}

/**
 Returns a randomized position on a plane in a radius around a given point
 
 @param center Center point for random distribution
 @param radius Maximum distance from center point
 @return Vector of a random position on a plane within a radius of center point
 */
struct vector getRandomVecOnPlane(struct vector center, double radius) {
	//FIXME: This only works in one orientation!
	return vectorWithPos(center.x + getRandomDouble(-radius, radius),
						 center.y + getRandomDouble(-radius, radius),
						 center.z);
}

struct vector multiplyVectors(struct vector v1, struct vector v2) {
	return (struct vector){v1.x * v2.x, v1.y * v2.y, v1.z * v2.z, false};
}

struct vector vectorMultiply(struct vector v, double c) {
	return (struct vector){v.x * c, v.y * c, v.z * c, false};
}

/**
Returns the reflected ray vector from a surface

@param I Incident vector normalized
@param N Normal vector normalized
@return Vector of the reflected ray vector from a surface
*/
struct vector reflect(const struct vector* I, const struct vector* N) {
	struct vector Imin2dotNI = vectorSubtract(I, scalarProduct(N, I) * 2.0);
	return multiplyVectors(*N, Imin2dotNI);
}
