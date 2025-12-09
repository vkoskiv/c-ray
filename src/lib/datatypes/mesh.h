//
//  mesh.h
//  c-ray
//
//  Created by Valtteri Koskivuori on 27/04/2017.
//  Copyright © 2017-2025 Valtteri Koskivuori. All rights reserved.
//

#pragma once

#include <v.h>
#include <c-ray/c-ray.h>
#include <datatypes/poly.h>
#include <common/vector.h>

struct mesh {
	struct vertex_buffer vbuf;
	struct poly *polygons;
	struct bvh *bvh;
	float surface_area;
	char *name;
	float rayOffset;
};

void mesh_free(struct mesh *mesh);
