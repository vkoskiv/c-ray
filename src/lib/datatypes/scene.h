//
//  scene.h
//  c-ray
//
//  Created by Valtteri Koskivuori on 28/02/2015.
//  Copyright © 2015-2025 Valtteri Koskivuori. All rights reserved.
//

#pragma once

#include <v.h>
#include <datatypes/mesh.h>
#include <common/texture.h>
#include <renderer/instance.h>
#include "camera.h"
#include <nodes/bsdfnode.h>

struct renderer;
struct hashtable;
struct file_cache;

struct node_storage {
	// Scene asset memory pool, currently used for nodes only.
	struct block *node_pool;
	// Used for hash consing. (preventing duplicate nodes)
	struct hashtable *node_table;
};

struct world {
	//Optional environment map / ambient color
	const struct bsdfNode *background;
	struct cr_shader_node *bg_desc;
	struct texture_asset_arr textures;
	struct bsdf_buffer_arr shader_buffers;
	struct mesh_arr meshes;
	struct instance_arr instances;
	bool instances_dirty; // Recompute top-level BVH?
	// Top-level bounding volume hierarchy,
	// contains all 3D assets in the scene.
	v_rwlock *bvh_lock;
	struct bvh *topLevel; // FIXME: Move to state?
	bool top_level_dirty;
	struct cr_thread_pool *bg_worker;

	struct sphere_arr spheres;
	struct camera_arr cameras;
	struct node_storage storage; // FIXME: Move to state?

	// c-ray is Y up, blender is Z up. This flag toggles
	// between the two in c-ray.
	bool use_blender_coordinates;

	char *asset_path;
};

struct world *scene_new(void);
void scene_destroy(struct world *scene);
