//
//  scene.c
//  c-ray
//
//  Created by Valtteri Koskivuori on 28/02/2015.
//  Copyright © 2015-2023 Valtteri Koskivuori. All rights reserved.
//

#include "../../includes.h"
#include "scene.h"

#include "../accelerators/bvh.h"
#include "../../common/hashtable.h"
#include "../../common/textbuffer.h"
#include "../../common/dyn_array.h"
#include "../../driver/node_parse.h" // FIXME: CROSS
#include "../../common/texture.h"
#include "camera.h"
#include "tile.h"
#include "../../driver/loaders/mesh.h" // FIXME: CROSS
#include "poly.h"

void tex_asset_free(struct texture_asset *a) {
	if (a->path) free(a->path);
	if (a->t) destroyTexture(a->t);
}

void scene_destroy(struct world *scene) {
	if (scene) {
		scene->textures.elem_free = tex_asset_free;
		texture_asset_arr_free(&scene->textures);
		camera_arr_free(&scene->cameras);
		for (size_t i = 0; i < scene->meshes.count; ++i) {
			destroyMesh(&scene->meshes.items[i]);
		}
		mesh_arr_free(&scene->meshes);
		destroy_bvh(scene->topLevel);
		destroyHashtable(scene->storage.node_table);
		destroyBlocks(scene->storage.node_pool);

		// TODO: find out a nicer way to bind elem_free to the array init
		scene->shader_buffers.elem_free = bsdf_buffer_free;
		bsdf_buffer_arr_free(&scene->shader_buffers);

		cr_shader_node_free(scene->bg_desc);
		// TODO: set as dyn_array elem_free somewhere
		for (size_t i = 0; i < scene->v_buffers.count; ++i) {
			vertex_buf_free(scene->v_buffers.items[i]);
		}
		vertex_buffer_arr_free(&scene->v_buffers);
		instance_arr_free(&scene->instances);
		sphere_arr_free(&scene->spheres);
		if (scene->asset_path) free(scene->asset_path);
		free(scene);
	}
}
