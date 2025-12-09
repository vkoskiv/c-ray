//
//  scene.c
//  c-ray
//
//  Created by Valtteri Koskivuori on 28/02/2015.
//  Copyright © 2015-2025 Valtteri Koskivuori. All rights reserved.
//

#include "../../includes.h"
#include "scene.h"

#include <v.h>
#include <accelerators/bvh.h>
#include <common/cr_string.h>
#include <common/hashtable.h>
#include <common/textbuffer.h>
#include <common/node_parse.h>
#include <common/texture.h>
#include "camera.h"
#include "mesh.h"

void tex_asset_free(struct texture_asset *a) {
	if (a->path) free(a->path);
	if (a->t) tex_destroy(a->t);
}

struct world *scene_new(void) {
	struct world *s = calloc(1, sizeof(*s));
	s->asset_path = stringCopy("./");
	s->storage.node_pool = newBlock(NULL, 1024);
	s->storage.node_table = newHashtable(compareNodes, &s->storage.node_pool);
	s->bvh_lock = v_rwlock_create();
	s->bg_worker = v_threadpool_create(v_sys_get_cores());
	return s;
}

void scene_destroy(struct world *scene) {
	if (scene) {
		// FIXME: elem_free
		// scene->textures.elem_free = tex_asset_free;
		for (size_t i = 0; i < v_arr_len(scene->textures); ++i)
			tex_asset_free(&scene->textures[i]);
		v_arr_free(scene->textures);
		v_arr_free(scene->cameras);
		// FIXME: elem_free
		// scene->meshes.elem_free = mesh_free;
		for (size_t i = 0; i < v_arr_len(scene->meshes); ++i)
			mesh_free(&scene->meshes[i]);
		v_arr_free(scene->meshes);

		v_rwlock_write_lock(scene->bvh_lock);
		destroy_bvh(scene->topLevel);
		v_rwlock_unlock(scene->bvh_lock);

		v_threadpool_destroy(scene->bg_worker);
		v_rwlock_destroy(scene->bvh_lock);

		destroyHashtable(scene->storage.node_table);
		destroyBlocks(scene->storage.node_pool);

		// TODO: find out a nicer way to bind elem_free to the array init
		// FIXME: elem_free
		// scene->shader_buffers.elem_free = bsdf_buffer_free;
		for (size_t i = 0; i < v_arr_len(scene->shader_buffers); ++i)
			bsdf_buffer_free(&scene->shader_buffers[i]);
		v_arr_free(scene->shader_buffers);

		cr_shader_node_free(scene->bg_desc);

		v_arr_free(scene->instances);
		v_arr_free(scene->spheres);
		if (scene->asset_path) free(scene->asset_path);
		free(scene);
	}
}
