//
//  poly.c
//  C-ray
//
//  Created by Valtteri Koskivuori on 28/02/2015.
//  Copyright © 2015-2019 Valtteri Koskivuori. All rights reserved.
//

#include "../includes.h"
#include "poly.h"
#include "../datatypes/vertexbuffer.h"

//Main polygon array
struct poly *polygonArray;
int polyCount;

bool rayIntersectsWithPolygon(struct lightRay *ray, struct poly *poly, float *result, struct vector *normal, struct coord *uv) {
	float orientation, inverseOrientation;
	struct vector edge1 = vecSubtract(vertexArray[poly->vertexIndex[1]], vertexArray[poly->vertexIndex[0]]);
	struct vector edge2 = vecSubtract(vertexArray[poly->vertexIndex[2]], vertexArray[poly->vertexIndex[0]]);
	
	//Find the cross product of edge 2 and the current ray direction
	struct vector s1 = vecCross(ray->direction, edge2);
	
	orientation = vecDot(edge1, s1);
	
	if (orientation > -0.000001 && orientation < 0.000001) {
		return false;
	}
	
	inverseOrientation = 1/orientation;
	
	struct vector s2 = vecSubtract(ray->start, vertexArray[poly->vertexIndex[0]]);
	float u = vecDot(s2, s1) * inverseOrientation;
	if (u < 0 || u > 1) {
		return false;
	}
	
	struct vector s3 = vecCross(s2, edge1);
	float v = vecDot(ray->direction, s3) * inverseOrientation;
	if (v < 0 || (u+v) > 1) {
		return false;
	}
	
	float temp = vecDot(edge2, s3) * inverseOrientation;
	
	if ((temp < 0) || (temp > *result)) {
		return false;
	}
	
	//For barycentric coordinates
	//Used for texturing and smooth shading
	*uv = (struct coord){u, v};
	
	*result = temp - 0.005; //This is to fix floating point precision error artifacts
	*normal = vecCross(edge2, edge1);
	*normal = vecNormalize(*normal);
	
	return true;
}
