//
//  poly.h
//  C-ray
//
//  Created by Valtteri Koskivuori on 28/02/2015.
//  Copyright © 2015-2019 Valtteri Koskivuori. All rights reserved.
//

#pragma once

struct poly {
	int vertexIndex[MAX_CRAY_VERTEX_COUNT];
	int normalIndex[MAX_CRAY_VERTEX_COUNT];
	int textureIndex[MAX_CRAY_VERTEX_COUNT];
	int materialIndex;
	int polyIndex;
	int vertexCount;
	bool hasNormals;
};

//Main polygon array
extern struct poly *polygonArray;
extern int polyCount;

struct lightRay;

//Calculates intersection between a light ray and a polygon object. Returns true if intersection has happened.
//result will be set to distance of intersect point, normal will be set to intersect normal, uv is the barycentric vec2 of that point
bool rayIntersectsWithPolygon(struct lightRay *ray, struct poly *poly, float *result, vec3 *normal, vec2 *uv);

//Just check for intersection
bool rayIntersectsWithPolygonFast(struct lightRay *ray, struct poly *poly);
