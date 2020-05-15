//
//  bbox.h
//  C-ray
//
//  Created by Valtteri Koskivuori on 28/04/2017.
//  Copyright © 2015-2020 Valtteri Koskivuori. All rights reserved.
//

#pragma once

#include "../datatypes/vector.h"
#include "../datatypes/lightRay.h"

/// Current bbox split axis
enum bboxAxis {
	X,
	Y,
	Z
};

/// Bounding box for a given set of primitives
struct boundingBox {
	struct vector start, end, midPoint;
	float surfaceArea;
};

/// Computes a bounding box for a given array of polygons
/// @param polys Array of polygons to process
/// @param count Amount of polygons given
struct boundingBox *computeBoundingBox(const int *polys, const int count);

/// Compute the longest axis of a given bounding box
/// @param bbox Bounding box to process
enum bboxAxis getLongestAxis(const struct boundingBox *bbox);

/// Checks  for an intersection between a given ray and bounding box
/// @param box Bounding box to check intersection against
/// @param ray Ray to intersect
bool rayIntersectsWithAABB(const struct boundingBox *box, const struct lightRay *ray);
