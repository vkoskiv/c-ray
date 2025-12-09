//
//  meshloader.h
//  c-ray
//
//  Created by Valtteri Koskivuori on 14.11.2019.
//  Copyright © 2019-2023 Valtteri Koskivuori. All rights reserved.
//

#pragma once

#include <c-ray/c-ray.h>
#include <v.h>
#include "../vector.h"

struct mesh_material {
	char *name;
	struct cr_shader_node *mat;
};

struct ext_mesh {
	struct vertex_buffer *vbuf;
	struct cr_face *faces;
	size_t vbuf_idx;
	float surface_area;
	float ray_offset;
	char *name;
};

static inline void ext_mesh_free(struct ext_mesh *m) {
	if (m->vbuf) vertex_buf_free(m->vbuf);
	v_arr_free(m->faces);
	if (m->name) free(m->name);
}

struct mesh_parse_result {
	struct ext_mesh *meshes;
	struct mesh_material *materials;
	struct vertex_buffer geometry;
};

struct mesh_parse_result load_meshes_from_file(const char *file_path);
