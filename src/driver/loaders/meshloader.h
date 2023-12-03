//
//  meshloader.h
//  c-ray
//
//  Created by Valtteri Koskivuori on 14.11.2019.
//  Copyright © 2019-2023 Valtteri Koskivuori. All rights reserved.
//

#pragma once

#include "mesh.h"

struct mesh_material {
	char *name;
	struct cr_shader_node *mat;
};

typedef struct mesh_material mesh_material;
dyn_array_def(mesh_material);

struct mesh_parse_result {
	struct mesh_arr meshes;
	struct mesh_material_arr materials;
	struct vertex_buffer geometry;
};

struct mesh_parse_result load_meshes_from_file(const char *file_path);
