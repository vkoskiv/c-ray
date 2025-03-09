//
//  poly.c
//  c-ray
//
//  Created by Valtteri Koskivuori on 28/02/2015.
//  Copyright © 2015-2025 Valtteri Koskivuori. All rights reserved.
//

#include "../../includes.h"
#include "poly.h"

#include "lightray.h"
#include <common/vector.h>
#include <renderer/pathtrace.h>
#include <datatypes/mesh.h>

bool rayIntersectsWithPolygon(const struct mesh *mesh, const struct lightRay *ray, const struct poly *poly, struct hitRecord *isect) {
	// Möller-Trumbore ray-triangle intersection routine
	// (see "Fast, Minimum Storage Ray-Triangle Intersection", by T. Möller and B. Trumbore)
	struct vector e1 = vec_sub(mesh->vbuf->vertices.items[poly->vertexIndex[0]], mesh->vbuf->vertices.items[poly->vertexIndex[1]]);
	struct vector e2 = vec_sub(mesh->vbuf->vertices.items[poly->vertexIndex[2]], mesh->vbuf->vertices.items[poly->vertexIndex[0]]);
	struct vector n = vec_cross(e1, e2);

	struct vector c = vec_sub(mesh->vbuf->vertices.items[poly->vertexIndex[0]], ray->start);
	struct vector r = vec_cross(ray->direction, c);
	float invDet = 1.0f / vec_dot(n, ray->direction);

	float u = vec_dot(r, e2) * invDet;
	float v = vec_dot(r, e1) * invDet;
	float w = 1.0f - u - v;

	// This order of comparisons guarantees that none of u, v, or t, are NaNs:
	// IEEE-754 mandates that they compare to false if the left hand side is a NaN.
	if (u >= 0.0f && v >= 0.0f && u + v <= 1.0f) {
		float t = vec_dot(n, c) * invDet;
		if (t >= 0.0f && t < isect->distance) {
			isect->uv = (struct coord) { u, v };
			isect->distance = t;
			if (likely(poly->hasNormals)) {
				struct vector upcomp = vec_scale(mesh->vbuf->normals.items[poly->normalIndex[1]], u);
				struct vector vpcomp = vec_scale(mesh->vbuf->normals.items[poly->normalIndex[2]], v);
				struct vector wpcomp = vec_scale(mesh->vbuf->normals.items[poly->normalIndex[0]], w);
				
				isect->surfaceNormal = vec_add(vec_add(upcomp, vpcomp), wpcomp);
			} else {
				isect->surfaceNormal = n;
			}
			// Support two-sided materials by flipping the normal if needed
			if (vec_dot(ray->direction, isect->surfaceNormal) >= 0.0f) isect->surfaceNormal = vec_negate(isect->surfaceNormal);
			isect->hitPoint = alongRay(ray, t);
			return true;
		}
	}
	return false;
}
