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

bool rayIntersectsWithPolygon(const struct lightRay *ray, const struct poly *poly, float *result, struct vector *normal, struct coord *uv) {
	// Moeller-Trumbore ray-triangle intersection routine
	// (see "Fast, Minimum Storage Ray-Triangle Intersection", by T. Moeller and B. Trumbore)
	struct vector e1 = vecSub(g_vertices[poly->vertexIndex[0]], g_vertices[poly->vertexIndex[1]]);
	struct vector e2 = vecSub(g_vertices[poly->vertexIndex[2]], g_vertices[poly->vertexIndex[0]]);
	struct vector n = vecCross(e1, e2);

	struct vector c = vecSub(g_vertices[poly->vertexIndex[0]], ray->start);
	struct vector r = vecCross(ray->direction, c);
	float invDet = 1.0f / vecDot(n, ray->direction);

	float u = vecDot(r, e2) * invDet;
	float v = vecDot(r, e1) * invDet;
	if (u < 0.0f || v < 0.0f || u + v > 1.0f)
		return false;

	float t = vecDot(n, c) * invDet;
	if (t < 0.0f || t > *result)
		return false;

	*uv = (struct coord) { u, v };
	*result = t;
	*normal = vecNormalize(n);
	return true;
}
