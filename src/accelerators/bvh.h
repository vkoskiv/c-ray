//
//  bvh.h
//  C-ray
//
//  Created by Arsène Pérard-Gayot on 07/06/2020.
//  Copyright © 2020-2022 Arsène Pérard-Gayot (@madmann91), Valtteri Koskivuori. All rights reserved.
//

#pragma once

#include "../renderer/samplers/sampler.h"
#include "../renderer/instance.h"

#include <stdbool.h>
#include <stddef.h>

struct lightRay;
struct hitRecord;
struct mesh;
struct poly;
struct boundingBox;

struct bvh;

/// Returns the bounding box of the root of the given BVH
struct boundingBox get_root_bbox(const struct bvh *bvh);

/// Builds a BVH for a given mesh
/// @param mesh Mesh containing polygons to process
/// @param count Amount of polygons given
struct bvh *build_mesh_bvh(const struct mesh *mesh);

/// Builds a top-level BVH for a given set of instances
/// @param instances Instances to build a top-level BVH for
/// @param instanceCount Amount of instances
struct bvh *build_top_level_bvh(const struct instance_arr instances);

/// Intersect a ray with a scene top-level BVH
bool traverse_top_level_bvh(
	const struct instance *instances,
	const struct bvh *bvh,
	const struct lightRay *ray,
	struct hitRecord *isect,
	sampler *sampler);

bool traverse_bottom_level_bvh(
	const struct mesh *mesh,
	const struct lightRay *ray,
	struct hitRecord *isect,
	sampler *sampler);

/// Frees the memory allocated by the given BVH
void destroy_bvh(struct bvh *);

void compute_accels(struct mesh_arr meshes);
