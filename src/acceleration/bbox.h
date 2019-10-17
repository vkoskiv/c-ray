//
//  bbox.h
//  C-ray
//
//  Created by Valtteri Koskivuori on 28/04/2017.
//  Copyright © 2015-2019 Valtteri Koskivuori. All rights reserved.
//

#pragma once

struct poly;

enum bboxAxis {
	X,
	Y,
	Z
};

struct boundingBox {
	struct vector start, end, midPoint;
};


/// Computes a bounding box for a given array of polygons
/// @param polys Array of polygons to process
/// @param count Amount of polygons given
struct boundingBox *computeBoundingBox(int *polys, int count);

/// Compute the longest axis of a given bounding box
/// @param bbox Bounding box to process
enum bboxAxis getLongestAxis(struct boundingBox *bbox);

/// Checks  for an intersection between a given ray and bounding box
/// @param box Bounding box to check intersection against
/// @param ray Ray to intersect
/// @param t Distance the intersection occurred at along the ray
bool rayIntersectWithAABB(struct boundingBox *box, struct lightRay *ray, float *t);
