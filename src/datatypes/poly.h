//
//  poly.h
//  C-ray
//
//  Created by Valtteri Koskivuori on 28/02/2015.
//  Copyright © 2015-2022 Valtteri Koskivuori. All rights reserved.
//

#pragma once

#include "../includes.h"

struct poly {
	int vertexIndex[MAX_CRAY_VERTEX_COUNT];
	int normalIndex[MAX_CRAY_VERTEX_COUNT];
	int textureIndex[MAX_CRAY_VERTEX_COUNT];
	unsigned int materialIndex: 16;
	unsigned int vertexCount: 3;
	bool hasNormals;
};

struct lightRay;
struct hitRecord;

//Calculates intersection between a light ray and a polygon object. Returns true if intersection has happened.
bool rayIntersectsWithPolygon(const struct lightRay *ray, const struct poly *poly, struct hitRecord *isect);
