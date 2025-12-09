//
//  poly.h
//  c-ray
//
//  Created by Valtteri Koskivuori on 28/02/2015.
//  Copyright © 2015-2025 Valtteri Koskivuori. All rights reserved.
//

#pragma once

#include "../../includes.h"
#include <v.h>
#include <c-ray/c-ray.h>

struct poly {
	int vertexIndex[MAX_CRAY_VERTEX_COUNT];
	int normalIndex[MAX_CRAY_VERTEX_COUNT];
	int textureIndex[MAX_CRAY_VERTEX_COUNT];
	unsigned int materialIndex: 16;
	bool hasNormals;
};

struct lightRay;
struct hitRecord;
struct mesh;

//Calculates intersection between a light ray and a polygon object. Returns true if intersection has happened.
bool rayIntersectsWithPolygon(const struct mesh *mesh, const struct lightRay *ray, const struct poly *poly, struct hitRecord *isect);
