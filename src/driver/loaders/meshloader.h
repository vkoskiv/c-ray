//
//  meshloader.h
//  C-ray
//
//  Created by Valtteri Koskivuori on 14.11.2019.
//  Copyright © 2019-2022 Valtteri Koskivuori. All rights reserved.
//

#pragma once

#include "../../datatypes/mesh.h"

struct mesh_parse_result {
	struct mesh_arr meshes;
	struct material_arr materials;
	struct vertex_buffer geometry;
};

struct mesh_parse_result load_meshes_from_file(const char *file_path);
