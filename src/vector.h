//
//  vector.h
//  C-Ray
//
//  Created by Valtteri Koskivuori on 28/02/15.
//  Copyright (c) 2015 Valtteri Koskivuori. All rights reserved.
//

#ifndef __C_Ray__vector__
#define __C_Ray__vector__

#include "includes.h"

//Vector
typedef struct {
	double x, y, z;
	//Polygons can share vertexes, so when we perform transforms
	//We want to avoid transforming a vector multiple times
	//So we keep track of that with the isTransformed flag.
	//This is reset after each transform, so all vertexes SHOULD
	//have this as FALSE when render starts.
	bool isTransformed;
}vector;

extern vector *vertexArray;
extern int vertexCount;
extern vector *normalArray;
extern int normalCount;
extern vector *textureArray;
extern int textureCount;

//Simulated light ray
typedef struct {
    vector start;
    vector direction;
}lightRay;

//Return a vector with given coordinates
vector vectorWithPos(double x, double y, double z);

//Add two vectors and return the resulting vector
vector addVectors(vector *v1, vector *v2);

//Subtract two vectors and return the resulting vector
vector subtractVectors(vector *v1, vector *v2);

//Multiply two vectors and return the dot product
float scalarProduct(vector *v1, vector *v2);

//Multiply a vector by a coefficient and return the resulting vector
vector vectorScale(double c, vector *v);

//Calculate the cross product of two vectors and return the resulting vector
vector vectorCross(vector *v1, vector *v2);

//Calculate min of 2 vectors
vector minVector(vector *v1, vector *v2);

//Calculate max of 2 vectors
vector maxVector(vector *v1, vector *v2);

//Calculate length of vector
float vectorLength(vector *v);

#endif /* defined(__C_Ray__vector__) */
