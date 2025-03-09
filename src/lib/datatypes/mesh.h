//
//  mesh.h
//  c-ray
//
//  Created by Valtteri Koskivuori on 27/04/2017.
//  Copyright © 2017-2025 Valtteri Koskivuori. All rights reserved.
//

#pragma once

#include <c-ray/c-ray.h>
#include <datatypes/poly.h>
#include <common/dyn_array.h>
#include <common/vector.h>

typedef struct cr_face cr_face;
dyn_array_def(cr_face)

struct mesh {
	struct vertex_buffer *vbuf;
	struct poly_arr polygons;
	struct bvh *bvh;
	size_t vbuf_idx;
	float surface_area;
	char *name;
	float rayOffset;
};

typedef struct mesh mesh;
dyn_array_def(mesh)

void mesh_free(struct mesh *mesh);
