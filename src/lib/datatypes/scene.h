//
//  scene.h
//  c-ray
//
//  Created by Valtteri Koskivuori on 28/02/2015.
//  Copyright © 2015-2023 Valtteri Koskivuori. All rights reserved.
//

#pragma once

#include <stddef.h>
#include "../../driver/loaders/mesh.h" // FIXME: CROSS
#include "../renderer/instance.h"
#include "camera.h"
#include "../../common/texture.h"
#include "../nodes/bsdfnode.h"

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
	struct vertex_buffer_arr v_buffers;
	struct bsdf_buffer_arr shader_buffers;
	struct mesh_arr meshes;
	struct instance_arr instances;
	// Top-level bounding volume hierarchy,
	// contains all 3D assets in the scene.
	struct bvh *topLevel; // FIXME: Move to state?
	struct sphere_arr spheres;
	struct camera_arr cameras;
	struct node_storage storage; // FIXME: Move to state?

	char *asset_path;
};

void scene_destroy(struct world *scene);
