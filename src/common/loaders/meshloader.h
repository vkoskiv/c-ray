//
//  meshloader.h
//  c-ray
//
//  Created by Valtteri Koskivuori on 14.11.2019.
//  Copyright © 2019-2023 Valtteri Koskivuori. All rights reserved.
//

#pragma once

#include <c-ray/c-ray.h>
#include "../../common/vector.h"

struct mesh_material {
	char *name;
	struct cr_shader_node *mat;
};

typedef struct mesh_material mesh_material;
dyn_array_def(mesh_material);

typedef struct cr_face cr_face;
dyn_array_def(cr_face);

struct ext_mesh {
	struct vertex_buffer *vbuf;
	struct cr_face_arr faces;
	size_t vbuf_idx;
	float surface_area;
	float ray_offset;
	char *name;
};

typedef struct ext_mesh ext_mesh;
dyn_array_def(ext_mesh);

static inline void ext_mesh_free(struct ext_mesh *m) {
	if (m->vbuf) vertex_buf_free(m->vbuf);
	cr_face_arr_free(&m->faces);
	if (m->name) free(m->name);
}

struct mesh_parse_result {
	struct ext_mesh_arr meshes;
	struct mesh_material_arr materials;
	struct vertex_buffer geometry;
};

struct mesh_parse_result load_meshes_from_file(const char *file_path);
