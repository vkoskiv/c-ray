//
//  mesh.h
//  C-ray
//
//  Created by Valtteri Koskivuori on 27/04/2017.
//  Copyright © 2017-2022 Valtteri Koskivuori. All rights reserved.
//

#pragma once

#include "../utils/dyn_array.h"
#include "vector.h"
#include "poly.h"
#include "material.h"

/*
	Several meshes may point to the same vertex buffer, shared among meshes in
	a wavefront file, for instance. A simple refcount is kept to make it easier
	to automatically free it when adding and removing meshes.
*/
struct vertex_buffer {
	struct vector_arr vertices;
	struct vector_arr normals;
	struct coord_arr texture_coords;
	size_t refs;
};

struct mesh {
	struct vertex_buffer *vbuf;
	struct poly_arr polygons;
	struct bvh *bvh;
	float surface_area;
	char *name;
	float rayOffset;
};

typedef struct mesh mesh;
dyn_array_def(mesh);

void destroyMesh(struct mesh *mesh);

struct vertex_buffer *vertex_buf_ref(struct vertex_buffer *buf);
void vertex_buf_unref(struct vertex_buffer *buf);
