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
struct vector *vertexArray;
int vertexCount;
struct vector *normalArray;
int normalCount;
struct vector *textureArray;
int textureCount;

/* Vector Functions */

//Create and return a vector with position values. Useful for hard-coded arrays.
struct vector vectorWithPos(double x, double y, double z) {
	return (struct vector){x, y, z, false};
}

//Add two vectors and return the resulting vector
struct vector addVectors(struct vector *v1, struct vector *v2) {
	return (struct vector){v1->x + v2->x, v1->y + v2->y, v1->z + v2->z, false};
}

//Compute length of a vector
double vectorLength(struct vector *v) {
	return sqrt(v->x*v->x + v->y*v->y + v->z*v->z);
}

//Subtract two vectors and return the resulting vector
struct vector subtractVectors(const struct vector *v1, const struct vector *v2) {
	return (struct vector){v1->x - v2->x, v1->y - v2->y, v1->z - v2->z, false};
}

//Multiply two vectors and return the dot product
double scalarProduct(const struct vector *v1, const struct vector *v2) {
	return v1->x * v2->x + v1->y * v2->y + v1->z * v2->z;
}

//Multiply a vector by a scalar and return the resulting vector
struct vector vectorScale(const double c, const struct vector *v) {
	return (struct vector){v->x * c, v->y * c, v->z * c, false};
}

//Calculate cross product and return the resulting vector
struct vector vectorCross(struct vector *v1, struct vector *v2) {
	return (struct vector){((v1->y * v2->z) - (v1->z * v2->y)),
		((v1->z * v2->x) - (v1->x * v2->z)),
		((v1->x * v2->y) - (v1->y * v2->x)), false};
}

struct vector minVector(struct vector *v1, struct vector *v2) {
	return (struct vector){min(v1->x, v2->x), min(v1->y, v2->y), min(v1->z, v2->z), false};
}

struct vector maxVector(struct vector *v1, struct vector *v2) {
	return (struct vector){max(v1->x, v2->x), max(v1->y, v2->y), max(v1->z, v2->z), false};
}

struct vector normalizeVector(struct vector *v) {
	double length = vectorLength(v);
	return (struct vector){v->x / length, v->y / length, v->z / length, false};
}

struct vector getMidPoint(struct vector *v1, struct vector *v2, struct vector *v3) {
	struct vector temp = addVectors(v1, v2);
	temp = addVectors(&temp, v3);
	return vectorScale(1.0/3.0, &temp);
}

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
