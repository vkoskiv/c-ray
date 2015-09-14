//
//  poly.c
//  C-Ray
//
//  Created by Valtteri Koskivuori on 28/02/15.
//  Copyright (c) 2015 Valtteri Koskivuori. All rights reserved.
//

#include "includes.h"

bool rayIntersectsWithPolygon(lightRay *ray, polygonObject *poly, double *result, vector *normal) {
	double det, invdet;
	vector edge1 = subtractVectors(&poly->v2, &poly->v1);
	vector edge2 = subtractVectors(&poly->v3, &poly->v1);
	
	//Find the cross product of edge 2 and the current ray direction
	vector s1 = vectorCross(&ray->direction, &edge2);
	
	det = scalarProduct(&edge1, &s1);
	//Prepare for floating point precision errors, find a better way to fix these!
	if (det > -0.000001 && det < 0.000001) {
		return false;
	}
	
	invdet = 1/det;
	
	vector s2 = subtractVectors(&ray->start, &poly->v1);
	double u = scalarProduct(&s2, &s1) * invdet;
	if (u < 0 || u > 1) {
		return false;
	}
	
	vector s3 = vectorCross(&s2, &edge1);
	double v = scalarProduct(&ray->direction, &s3) * invdet;
	if (v < 0 || (u+v) > 1) {
		return false;
	}
	
	double temp = scalarProduct(&edge2, &s3) * invdet;
	
	if ((temp < 0) || (temp > *result)) {
		return false;
	}
	
    *result = temp - 0.005; //This is to fix floating point precision error artifacts
	*normal = vectorCross(&edge2, &edge1);
	
	return true;
}