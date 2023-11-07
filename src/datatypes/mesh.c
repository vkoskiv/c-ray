//
//  mesh.c
//  C-ray
//
//  Created by Valtteri Koskivuori on 27/04/2017.
//  Copyright © 2017-2022 Valtteri Koskivuori. All rights reserved.
//

#include "../includes.h"
#include "mesh.h"

#include "../accelerators/bvh.h"
#include "poly.h"
#include "material.h"

dyn_array_def(mesh);

void destroyMesh(struct mesh *mesh) {
	if (mesh) {
		free(mesh->name);
		vector_arr_free(&mesh->vertices);
		vector_arr_free(&mesh->normals);
		coord_arr_free(&mesh->texture_coords);
		poly_arr_free(&mesh->polygons);
		destroy_bvh(mesh->bvh);
		if (mesh->materials.count) {
			for (size_t i = 0; i < mesh->materials.count; ++i) {
				destroyMaterial(&mesh->materials.items[i]);
			}
			material_arr_free(&mesh->materials);
		}
	}
}
