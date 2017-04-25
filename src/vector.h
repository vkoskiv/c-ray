//
//  vector.h
//  C-Ray
//
//  Created by Valtteri Koskivuori on 28/02/15.
//  Copyright (c) 2015 Valtteri Koskivuori. All rights reserved.
//

#pragma once

//Vector
struct vector {
	double x, y, z;
	//Polygons can share vertexes, so when we perform transforms
	//We want to avoid transforming a vector multiple times
	//So we keep track of that with the isTransformed flag.
	//This is reset after each transform, so all vertexes SHOULD
	//have this as FALSE when render starts.
	bool isTransformed;
};

//Main vector arrays
extern struct vector *vertexArray;
extern int vertexCount;

extern struct vector *normalArray;
extern int normalCount;

extern struct vector *textureArray;
extern int textureCount;

enum type {
	rayTypeIncident,
	rayTypeReflected,
	rayTypeRefracted,
	rayTypeShadow
};

//Simulated light ray
struct lightRay {
	struct vector start;
	struct vector direction;
	enum type rayType;
};

//Return a vector with given coordinates
struct vector vectorWithPos(double x, double y, double z);

//Add two vectors and return the resulting vector
struct vector addVectors(struct vector *v1, struct vector *v2);

//Subtract two vectors and return the resulting vector
struct vector subtractVectors(struct vector *v1, struct vector *v2);

//Multiply two vectors and return the dot product
float scalarProduct(struct vector *v1, struct vector *v2);

//Multiply a vector by a coefficient and return the resulting vector
struct vector vectorScale(double c, struct vector *v);

//Calculate the cross product of two vectors and return the resulting vector
struct vector vectorCross(struct vector *v1, struct vector *v2);

//Calculate min of 2 vectors
struct vector minVector(struct vector *v1, struct vector *v2);

//Calculate max of 2 vectors
struct vector maxVector(struct vector *v1, struct vector *v2);

//Calculate length of vector
float vectorLength(struct vector *v);

//Normalize a vector
struct vector normalizeVector(struct vector *v);
