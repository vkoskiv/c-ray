//
//  scene.h
//  C-ray
//
//  Created by Valtteri Koskivuori on 28/02/2015.
//  Copyright © 2015-2022 Valtteri Koskivuori. All rights reserved.
//

#pragma once

#include <stddef.h>

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
	float backgroundOffset;
	
	struct mesh *meshes;
	int meshCount;
	
	struct instance *instances;
	int instanceCount;
	
	// Top-level bounding volume hierarchy,
	// contains all 3D assets in the scene.
	struct bvh *topLevel;
	
	struct sphere *spheres;
	int sphereCount;

	struct camera *cameras;
	size_t camera_count;
	
	struct node_storage storage;
};

int loadScene(struct renderer *r, const char *input);

void destroyScene(struct world *scene);
