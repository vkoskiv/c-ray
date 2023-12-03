//
//  mesh.h
//  C-ray
//
//  Created by Valtteri Koskivuori on 27/04/2017.
//  Copyright © 2017-2022 Valtteri Koskivuori. All rights reserved.
//

#pragma once

#include "../../common/dyn_array.h"
#include "../../common/vector.h"
#include "../../lib/datatypes/poly.h" // FIXME: CROSS

struct vertex_buffer {
	struct vector_arr vertices;
	struct vector_arr normals;
	struct coord_arr texture_coords;
};

typedef struct vertex_buffer vertex_buffer;
dyn_array_def(vertex_buffer);

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
dyn_array_def(mesh);

void destroyMesh(struct mesh *mesh);

void vertex_buf_free(struct vertex_buffer buf);
