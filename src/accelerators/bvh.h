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

struct bvhNode {
	float bounds[6]; // Node bounds (min x, max x, min y, max y, ...)
	unsigned firstChildOrPrim; // Index to the first child or primitive (if the node is a leaf)
	unsigned primCount : 30;
	bool isLeaf : 1;
};

struct bvh {
	struct bvhNode* nodes;
	int *primIndices;
	unsigned nodeCount;
};

/// Builds a BVH
/// @param polygons Array of polygons to process
/// @param count Amount of polygons given
struct bvh *buildBvh(int *polygons, unsigned count);

struct bvh *topLevelBvh(struct mesh *meshes, unsigned meshCount);

/// Intersects a ray with the given BVH
bool rayIntersectsWithBvh(const struct bvh *bvh, const struct lightRay *ray, struct hitRecord *isect);

/// Frees the memory allocated by the given BVH
void destroyBvh(struct bvh *);
