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

struct mesh {
	struct vector_arr vertices;
	struct vector_arr normals;
	struct coord_arr texture_coords;
	struct poly_arr polygons;
	struct material_arr materials;
	struct bvh *bvh;
	float surface_area;
	char *name;
	float rayOffset;
};

typedef struct mesh mesh;
dyn_array_dec(mesh);

void destroyMesh(struct mesh *mesh);
