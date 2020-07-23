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
struct poly;
struct instance;

struct bvh;

/// Builds a BVH for a given set of polygons
/// @param polygons Array of polygons to process
/// @param count Amount of polygons given
struct bvh *buildBottomLevelBvh(struct poly *polys, unsigned count);

/// Builds a top-level BVH for a given set of instances
/// @param instances Instances to build a top-level BVH for
/// @param instanceCount Amount of instances
struct bvh *buildTopLevelBvh(struct instance *instances, unsigned instanceCount);

/// Intersect a ray with a scene top-level BVH
bool traverseTopLevelBvh(const struct instance *instances, const struct bvh *bvh, const struct lightRay *ray, struct hitRecord *isect);

bool traverseBottomLevelBvh(const struct mesh *mesh, const struct lightRay *ray, struct hitRecord *isect);

/// Frees the memory allocated by the given BVH
void destroyBvh(struct bvh *);
