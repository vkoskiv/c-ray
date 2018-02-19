//
//  vector.h
//  C-Ray
//
//  Created by Valtteri Koskivuori on 28/02/15.
//  Copyright (c) 2015 Valtteri Koskivuori. All rights reserved.
//

#pragma once

//Polygons can share vertexes, so when we perform transforms
//We want to avoid transforming a vector multiple times
//So we keep track of that with the isTransformed flag.
//This is reset after each transform, so all vertexes SHOULD
//have this as FALSE when render starts.

//Vector
struct vector {
	double x, y, z;
	//FIXME: Shouldn't need this here
	bool isTransformed;
};

struct coord {
	double x, y;
};

//Main vector arrays
extern struct vector *vertexArray;
extern int vertexCount;

extern struct vector *normalArray;
extern int normalCount;

extern struct vector *textureArray;
extern int textureCount;

//Return a vector with given coordinates
struct vector vectorWithPos(double x, double y, double z);

//Add two vectors and return the resulting vector
struct vector addVectors(struct vector *v1, struct vector *v2);

//Subtract two vectors and return the resulting vector
struct vector subtractVectors(const struct vector *v1, const struct vector *v2);

//Multiply two vectors and return the dot product
double scalarProduct(const struct vector *v1, const struct vector *v2);

//Multiply a vector by a coefficient and return the resulting vector
struct vector vectorScale(const double c, const struct vector *v);

//Calculate the cross product of two vectors and return the resulting vector
struct vector vectorCross(struct vector *v1, struct vector *v2);

//Calculate min of 2 vectors
struct vector minVector(struct vector *v1, struct vector *v2);

//Calculate max of 2 vectors
struct vector maxVector(struct vector *v1, struct vector *v2);

//Calculate length of vector
double vectorLength(struct vector *v);

//Normalize a vector
struct vector normalizeVector(struct vector *v);

struct coord uvFromValues(double u, double v);

struct vector getMidPoint(struct vector *v1, struct vector *v2, struct vector *v3);

struct vector getRandomVecOnRadius(struct vector center, double radius);

struct vector getRandomVecOnPlane(struct vector center, double radius);

double getRandomDouble(double min, double max);

struct vector vectorMultiply(struct vector v, const double c);

struct vector multiplyVectors(struct vector v1, struct vector v2);

struct vector reflect(const struct vector* I, const struct vector* N);