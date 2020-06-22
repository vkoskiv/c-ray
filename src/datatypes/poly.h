//
//  poly.h
//  C-ray
//
//  Created by Valtteri Koskivuori on 28/02/2015.
//  Copyright © 2015-2020 Valtteri Koskivuori. All rights reserved.
//

#pragma once

struct poly {
	int vertexIndex[MAX_CRAY_VERTEX_COUNT];
	int normalIndex[MAX_CRAY_VERTEX_COUNT];
	int textureIndex[MAX_CRAY_VERTEX_COUNT];
	int materialIndex: 16;
	int vertexCount: 3;
	bool hasNormals;
};

struct lightRay;

struct vector;
struct coord;

//Calculates intersection between a light ray and a polygon object. Returns true if intersection has happened.
//result will be set to distance of intersect point, normal will be set to intersect normal, uv is the barycentric coord of that point
bool rayIntersectsWithPolygon(const struct lightRay *ray, const struct poly *poly, float *result, struct vector *normal, struct coord *uv);
