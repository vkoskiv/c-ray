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
}vector;

extern int vertexCount;
extern int normalCount;
extern int textureCount;
extern vector *vertexArray;
extern vector *normalArray;
extern vector *textureArray;

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

#endif /* defined(__C_Ray__vector__) */
