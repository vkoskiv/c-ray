//
//  scene.c
//  c-ray
//
//  Created by Valtteri Koskivuori on 28/02/2015.
//  Copyright © 2015-2023 Valtteri Koskivuori. All rights reserved.
//

#include "../includes.h"
#include "scene.h"

#include "../accelerators/bvh.h"
#include "../utils/hashtable.h"
#include "../utils/args.h"
#include "../utils/textbuffer.h"
#include "../utils/dyn_array.h"
#include "camera.h"
#include "tile.h"
#include "mesh.h"
#include "poly.h"

//Free scene data
void destroyScene(struct world *scene) {
	if (scene) {
		camera_arr_free(&scene->cameras);
		for (size_t i = 0; i < scene->meshes.count; ++i) {
			destroyMesh(&scene->meshes.items[i]);
		}
		mesh_arr_free(&scene->meshes);
		destroy_bvh(scene->topLevel);
		destroyHashtable(scene->storage.node_table);
		destroyBlocks(scene->storage.node_pool);
		for (size_t i = 0; i < scene->instances.count; ++i) {
			bsdf_buf_unref(scene->instances.items[i].bbuf);
		}
		instance_arr_free(&scene->instances);
		sphere_arr_free(&scene->spheres);
		free(scene);
	}
}
