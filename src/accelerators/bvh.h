//
//  bvh.h
//  C-ray
//
//  Created by Arsène Pérard-Gayot on 07/06/2020.
//  Copyright © 2020 Arsène Pérard-Gayot (@madmann91). All rights reserved.
//

#pragma once

#include <stdbool.h>

struct lightRay;
struct hitRecord;
struct mesh;

struct bvh;

/// Builds a BVH for a given set of polygons
/// @param polygons Array of polygons to process
/// @param count Amount of polygons given
struct bvh *buildBottomLevelBvh(int *polygons, unsigned count);

/// Builds a top-level BVH for a given set of meshes
/// @param meshes Meshes to build a top-level BVH for
/// @param meshCount Amount of meshes
struct bvh *buildTopLevelBvh(struct mesh *meshes, unsigned meshCount);

/// Intersect a ray with a scene top-level BVH
bool traverseTopLevelBvh(const struct mesh *meshes, const struct bvh *bvh, const struct lightRay *ray, struct hitRecord *isect);

/// Frees the memory allocated by the given BVH
void destroyBvh(struct bvh *);
