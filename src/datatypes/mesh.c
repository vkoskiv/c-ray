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
#include "vector.h"

void destroyMesh(struct mesh *mesh) {
	if (mesh) {
		free(mesh->name);
		poly_arr_free(&mesh->polygons);
		destroy_bvh(mesh->bvh);
	}
}

void vertex_buf_free(struct vertex_buffer buf) {
	vector_arr_free(&buf.vertices);
	vector_arr_free(&buf.normals);
	coord_arr_free(&buf.texture_coords);
}
