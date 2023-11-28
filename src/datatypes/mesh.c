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
#include "vector.h"

void destroyMesh(struct mesh *mesh) {
	if (mesh) {
		free(mesh->name);
		vertex_buf_unref(mesh->vbuf);
		poly_arr_free(&mesh->polygons);
		destroy_bvh(mesh->bvh);
	}
}

struct vertex_buffer *vertex_buf_ref(struct vertex_buffer *buf) {
	if (buf) {
		buf->refs++;
		return buf;
	}
	struct vertex_buffer *new = calloc(1, sizeof(*new));
	new->refs = 1;
	return new;
}

void vertex_buf_unref(struct vertex_buffer *buf) {
	if (!buf) return;
	if (--buf->refs) return;
	vector_arr_free(&buf->vertices);
	vector_arr_free(&buf->normals);
	coord_arr_free(&buf->texture_coords);
	free(buf);
}
