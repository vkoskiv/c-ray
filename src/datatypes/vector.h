//
//  vector.h
//  C-ray
//
//  Created by Valtteri Koskivuori on 28/02/2015.
//  Copyright © 2015-2019 Valtteri Koskivuori. All rights reserved.
//

#pragma once

struct vector {
	double x, y, z;
};

typedef struct vector vector;

struct coord {
	double x, y;
};

//Main vector arrays
extern struct vector *vertexArray;
extern int vertexCount;

extern struct vector *normalArray;
extern int normalCount;

extern struct coord *textureArray;
extern int textureCount;

//Return a vector with given coordinates
struct vector vecWithPos(double x, double y, double z);

//Add two vectors and return the resulting vector
struct vector vecAdd(struct vector *v1, struct vector *v2);

//Subtract two vectors and return the resulting vector
struct vector vecSubtract(const struct vector *v1, const struct vector *v2);

//Multiply two vectors and return the dot product
double vecDot(const struct vector *v1, const struct vector *v2);

//Multiply a vector by a coefficient and return the resulting vector
struct vector vecScale(const double c, const struct vector *v);

struct coord coordScale(const double c, const struct coord *crd);
struct coord addCoords(const struct coord *c1, const struct coord *c2);

//Calculate the cross product of two vectors and return the resulting vector
struct vector vecCross(struct vector *v1, struct vector *v2);

//Calculate min of 2 vectors
struct vector vecMin(struct vector *v1, struct vector *v2);

//Calculate max of 2 vectors
struct vector vecMax(struct vector *v1, struct vector *v2);

//Calculate length of vector
double vecLength(struct vector *v);

//calculate length^2 of vector
double vecLengthSquared(struct vector *v);

//Normalize a vector
struct vector vecNormalize(struct vector *v);

struct coord uvFromValues(double u, double v);

struct vector getMidPoint(struct vector *v1, struct vector *v2, struct vector *v3);

struct vector getRandomVecOnRadius(struct vector center, double radius, pcg32_random_t *rng);

struct vector getRandomVecOnPlane(struct vector center, double radius, pcg32_random_t *rng);

double rndDouble(double min, double max, pcg32_random_t *rng);

struct vector vecMultiplyConst(struct vector v, const double c);

struct vector vecMultiply(struct vector v1, struct vector v2);

struct vector reflect(const struct vector* I, const struct vector* N);

struct vector vecNegate(struct vector v);
