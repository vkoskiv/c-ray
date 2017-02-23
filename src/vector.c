//
//  vector.c
//  C-Ray
//
//  Created by Valtteri Koskivuori on 28/02/15.
//  Copyright (c) 2015 Valtteri Koskivuori. All rights reserved.
//

#include "vector.h"

//Main vertex arrays
vector *vertexArray;
int vertexCount;
vector *normalArray;
int normalCount;
vector *textureArray;
int textureCount;

/* Vector Functions */

//Create and return a vector with position values. Useful for hard-coded arrays.
vector vectorWithPos(double x, double y, double z) {
	return (vector){x, y, z};
}

//Add two vectors and return the resulting vector
vector addVectors(vector *v1, vector *v2) {
	return (vector){v1->x + v2->x, v1->y + v2->y, v1->z + v2->z};
}

//Compute length of a vector
float vectorLength(vector *v) {
    return sqrtf(v->x*v->x + v->y*v->y + v->z*v->z);
}

//Subtract two vectors and return the resulting vector
vector subtractVectors(vector *v1, vector *v2) {
	return (vector){v1->x - v2->x, v1->y - v2->y, v1->z - v2->z};
}

//Multiply two vectors and return the dot product
float scalarProduct(vector *v1, vector *v2) {
	return v1->x * v2->x + v1->y * v2->y + v1->z * v2->z;
}

//Multiply a vector by a scalar and return the resulting vector
vector vectorScale(double c, vector *v) {
	return (vector){v->x * c, v->y * c, v->z * c};
}

//Calculate cross product and return the resulting vector
vector vectorCross(vector *v1, vector *v2) {
	vector result;
	
	result.x = (v1->y * v2->z) - (v1->z * v2->y);
	result.y = (v1->z * v2->x) - (v1->x * v2->z);
	result.z = (v1->x * v2->y) - (v1->y * v2->x);
	
	return result;
}

vector minVector(vector *v1, vector *v2) {
	return (vector){min(v1->x, v2->x), min(v1->y, v2->y), min(v1->z, v2->z)};
}

vector maxVector(vector *v1, vector *v2) {
	return (vector){max(v1->x, v2->x), max(v1->y, v2->y), max(v1->z, v2->z)};
}
