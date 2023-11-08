//
//  scene.h
//  C-ray
//
//  Created by Valtteri Koskivuori on 28/02/2015.
//  Copyright © 2015-2022 Valtteri Koskivuori. All rights reserved.
//

#pragma once

#include <stddef.h>
#include "mesh.h"

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
	struct mesh_arr meshes;
	struct instance *instances;
	// Top-level bounding volume hierarchy,
	// contains all 3D assets in the scene.
	struct bvh *topLevel;
	struct sphere *spheres;
	struct camera *cameras;
	struct node_storage storage;
	float backgroundOffset;
	int instanceCount;
	int sphereCount;
	size_t camera_count;
};

int loadScene(struct renderer *r, char *input);

void destroyScene(struct world *scene);
