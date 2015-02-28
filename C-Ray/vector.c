//
//  vector.c
//  C-Ray
//
//  Created by Valtteri Koskivuori on 28/02/15.
//  Copyright (c) 2015 Valtteri Koskivuori. All rights reserved.
//

#include "vector.h"

/* Vector Functions */

//Add two vectors and return the resulting vector
vector addVectors(vector *v1, vector *v2) {
	vector result = {v1->x + v2->x, v1->y + v2->y, v1->z + v2->z};
	return result;
}

//Subtract two vectors and return the resulting vector
vector subtractVectors(vector *v1, vector *v2) {
	vector result = {v1->x - v2->x, v1->y - v2->y, v1->z - v2->z };
	return result;
}

//Multiply two vectors and return the dot product
float scalarProduct(vector *v1, vector *v2) {
	return v1->x * v2->x + v1->y * v2->y + v1->z * v2->z;
}

//Multiply a vector by a scalar and return the resulting vector
vector vectorScale(double c, vector *v) {
	vector result = {v->x * c, v->y * c, v->z * c};
	return result;
}

//Calculate cross product and return the resulting vector
vector vectorCross(vector *v1, vector *v2) {
	vector result;
	
	result.x = (v1->y * v2->z) - (v1->z * v2->y);
	result.y = (v1->z * v2->x) - (v1->x * v2->z);
	result.z = (v1->x * v2->y) - (v1->y * v2->x);
	
	return result;
}