//
//  mesh.c
//  C-ray
//
//  Created by Valtteri Koskivuori on 27/04/2017.
//  Copyright © 2017-2022 Valtteri Koskivuori. All rights reserved.
//

#include "../../includes.h"
#include "mesh.h"

#include "../accelerators/bvh.h"
#include "../../common/vector.h"

void mesh_free(struct mesh *mesh) {
	if (mesh) {
		free(mesh->name);
		poly_arr_free(&mesh->polygons);
		destroy_bvh(mesh->bvh);
	}
}
