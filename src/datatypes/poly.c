//
//  poly.c
//  C-ray
//
//  Created by Valtteri Koskivuori on 28/02/2015.
//  Copyright © 2015-2020 Valtteri Koskivuori. All rights reserved.
//

#include "../includes.h"
#include "poly.h"
#include "vertexbuffer.h"

#include "vector.h"
#include "lightRay.h"

//Main polygon array
struct poly *polygonArray;
int polyCount;

bool rayIntersectsWithPolygon(struct lightRay *ray, struct poly *poly, float *result, struct vector *normal, struct coord *uv) {
	float orientation, inverseOrientation;
	struct vector edge1 = vecSub(vertexArray[poly->vertexIndex[2]], vertexArray[poly->vertexIndex[0]]);
	struct vector edge2 = vecSub(vertexArray[poly->vertexIndex[1]], vertexArray[poly->vertexIndex[0]]);
	
	//Find the cross product of edge 2 and the current ray direction
	struct vector s1 = vecCross(ray->direction, edge2);
	
	orientation = vecDot(edge1, s1);
	
	if (orientation > -0.00000001f && orientation < 0.00000001f) {
		return false;
	}
	
	inverseOrientation = 1.0f/orientation;
	
	struct vector s2 = vecSub(ray->start, vertexArray[poly->vertexIndex[0]]);
	float u = vecDot(s2, s1) * inverseOrientation;
	if (u < 0.0f || u > 1.0f) {
		return false;
	}
	
	struct vector s3 = vecCross(s2, edge1);
	float v = vecDot(ray->direction, s3) * inverseOrientation;
	if (v < 0.0f || (u+v) > 1.0f) {
		return false;
	}
	
	float temp = vecDot(edge2, s3) * inverseOrientation;
	
	if ((temp < 0.0f) || (temp > *result)) {
		return false;
	}
	
	//For barycentric coordinates
	//Used for texturing and smooth shading
	*uv = (struct coord){u, v};
	*result = temp;
	*normal = vecNormalize(vecCross(edge2, edge1));
	return true;
}
