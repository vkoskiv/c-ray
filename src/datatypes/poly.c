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

bool rayIntersectsWithPolygonFast(struct lightRay *ray, struct poly *poly) {
	float orientation, inverseOrientation;
	vec3 edge1 = vecSubtract(vertexArray[poly->vertexIndex[1]], vertexArray[poly->vertexIndex[0]]);
	vec3 edge2 = vecSubtract(vertexArray[poly->vertexIndex[2]], vertexArray[poly->vertexIndex[0]]);
	
	//Find the cross product of edge 2 and the current ray direction
	vec3 s1 = vecCross(ray->direction, edge2);
	
	orientation = vecDot(edge1, s1);
	//Prepare for floating point precision errors, find a better way to fix these!
	if (orientation > -0.000001 && orientation < 0.000001) {
		return false;
	}
	
	inverseOrientation = 1/orientation;
	
	vec3 s2 = vecSubtract(ray->start, vertexArray[poly->vertexIndex[0]]);
	float u = vecDot(s2, s1) * inverseOrientation;
	if (u < 0 || u > 1) {
		return false;
	}
	
	vec3 s3 = vecCross(s2, edge1);
	float v = vecDot(ray->direction, s3) * inverseOrientation;
	if (v < 0 || (u+v) > 1) {
		return false;
	}
	
	float temp = vecDot(edge2, s3) * inverseOrientation;
	
	if (temp < 0) {
		return false;
	}
	
	return true;
}

bool rayIntersectsWithPolygon(struct lightRay *ray, struct poly *poly, float *result, vec3 *normal, vec2 *uv) {
	float orientation, inverseOrientation;
	vec3 edge1 = vec3_sub(vertexArray[poly->vertexIndex[1]], vertexArray[poly->vertexIndex[0]]);
	vec3 edge2 = vec3_sub(vertexArray[poly->vertexIndex[2]], vertexArray[poly->vertexIndex[0]]);
	
	//Find the cross product of edge 2 and the current ray direction
	vec3 s1 = vec3_cross(ray->direction, edge2);
	
	orientation = vec3_dot(edge1, s1);
	
	if (orientation > -0.000001 && orientation < 0.000001) {
		return false;
	}
	
	inverseOrientation = 1/orientation;
	
	vec3 s2 = vec3_sub(ray->start, vertexArray[poly->vertexIndex[0]]);
	float u = vec3_dot(s2, s1) * inverseOrientation;
	if (u < 0 || u > 1) {
		return false;
	}
	
	vec3 s3 = vec3_cross(s2, edge1);
	float v = vec3_dot(ray->direction, s3) * inverseOrientation;
	if (v < 0 || (u+v) > 1) {
		return false;
	}
	
	float temp = vec3_dot(edge2, s3) * inverseOrientation;
	
	if ((temp < 0) || (temp > *result)) {
		return false;
	}
	
	//For barycentric vec2inates
	//Used for texturing and smooth shading
	*uv = (vec2){u, v};
	
	*result = temp - 0.005; //This is to fix floating point precision error artifacts
	*normal = vec3_cross(edge1, edge2);
	*normal = vec3_normalize(*normal);
	
	return true;
}
