//
//  poly.c
//  C-Ray
//
//  Created by Valtteri Koskivuori on 28/02/15.
//  Copyright (c) 2015 Valtteri Koskivuori. All rights reserved.
//

#include "includes.h"

bool rayIntersectsWithPolygon(lightRay *r, polygonObject *t, double *result, vector *normal) {
	double det, invdet;
	vector edge1 = subtractVectors(&t->v2, &t->v1);
	vector edge2 = subtractVectors(&t->v3, &t->v1);
	
	//Find the cross product of edge 2 and the current ray direction
	vector s1 = vectorCross(&r->direction, &edge2);
	
	det = scalarProduct(&edge1, &s1);
	//Prepare for floating point precision errors, find a better way to fix these!
	if (det > -0.000001 && det < 0.000001) {
		return false;
	}
	
	invdet = 1/det;
	
	vector s2 = subtractVectors(&r->start, &t->v1);
	double u = scalarProduct(&s2, &s1) * invdet;
	if (u < 0 || u > 1) {
		return false;
	}
	
	vector s3 = vectorCross(&s2, &edge1);
	double v = scalarProduct(&r->direction, &s3) * invdet;
	if (v < 0 || (u+v) > 1) {
		return false;
	}
	
	double temp = scalarProduct(&edge2, &s3) * invdet;
	
	if (((temp < 0) || (temp > *result))) {
		return false;
	}
	
	*result = temp - 0.002; //This is to fix floating point precision error artifacts
	*normal = vectorCross(&edge2, &edge1);
	
	return true;
}