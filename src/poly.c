//
//  poly.c
//  C-Ray
//
//  Created by Valtteri Koskivuori on 28/02/15.
//  Copyright (c) 2015 Valtteri Koskivuori. All rights reserved.
//

#include "includes.h"
#include "poly.h"

//Main polygon array
struct poly *polygonArray;
int polyCount;

bool rayIntersectsWithPolygonFast(struct lightRay *ray, struct poly *poly) {
	double orientation, inverseOrientation;
	struct vector edge1 = subtractVectors(&vertexArray[poly->vertexIndex[1]], &vertexArray[poly->vertexIndex[0]]);
	struct vector edge2 = subtractVectors(&vertexArray[poly->vertexIndex[2]], &vertexArray[poly->vertexIndex[0]]);
	
	//Find the cross product of edge 2 and the current ray direction
	struct vector s1 = vectorCross(&ray->direction, &edge2);
	
	orientation = scalarProduct(&edge1, &s1);
	//Prepare for floating point precision errors, find a better way to fix these!
	if (orientation > -0.000001 && orientation < 0.000001) {
		return false;
	}
	
	inverseOrientation = 1/orientation;
	
	struct vector s2 = subtractVectors(&ray->start, &vertexArray[poly->vertexIndex[0]]);
	double u = scalarProduct(&s2, &s1) * inverseOrientation;
	if (u < 0 || u > 1) {
		return false;
	}
	
	struct vector s3 = vectorCross(&s2, &edge1);
	double v = scalarProduct(&ray->direction, &s3) * inverseOrientation;
	if (v < 0 || (u+v) > 1) {
		return false;
	}
	
	double temp = scalarProduct(&edge2, &s3) * inverseOrientation;
	
	if (temp < 0) {
		return false;
	}
	
	return true;
}

bool rayIntersectsWithPolygon(struct lightRay *ray, struct poly *poly, double *result, struct vector *normal, struct coord *uv) {
	double orientation, inverseOrientation;
	struct vector edge1 = subtractVectors(&vertexArray[poly->vertexIndex[1]], &vertexArray[poly->vertexIndex[0]]);
	struct vector edge2 = subtractVectors(&vertexArray[poly->vertexIndex[2]], &vertexArray[poly->vertexIndex[0]]);
	
	//Find the cross product of edge 2 and the current ray direction
	struct vector s1 = vectorCross(&ray->direction, &edge2);
	
	orientation = scalarProduct(&edge1, &s1);
	//Prepare for floating point precision errors, find a better way to fix these!
	if (orientation > -0.000001 && orientation < 0.000001) {
		return false;
	}
	
	inverseOrientation = 1/orientation;
	
	struct vector s2 = subtractVectors(&ray->start, &vertexArray[poly->vertexIndex[0]]);
	double u = scalarProduct(&s2, &s1) * inverseOrientation;
	if (u < 0 || u > 1) {
		return false;
	}
	
	struct vector s3 = vectorCross(&s2, &edge1);
	double v = scalarProduct(&ray->direction, &s3) * inverseOrientation;
	if (v < 0 || (u+v) > 1) {
		return false;
	}
	
	double temp = scalarProduct(&edge2, &s3) * inverseOrientation;
	
	if ((temp < 0) || (temp > *result)) {
		return false;
	}
	
	//For barycentric coordinates
	//Used for texturing and smooth shading
	*uv = uvFromValues(u, v);
	
	*result = temp - 0.005; //This is to fix floating point precision error artifacts
	*normal = vectorCross(&edge2, &edge1);
	
	return true;
}
