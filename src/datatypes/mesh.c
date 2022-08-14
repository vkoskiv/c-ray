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

void destroyMesh(struct mesh *mesh) {
	if (mesh) {
		free(mesh->name);
		if (mesh->vertices) free(mesh->vertices);
		if (mesh->normals) free(mesh->normals);
		if (mesh->texture_coords) free(mesh->texture_coords);
		if (mesh->polygons) free(mesh->polygons);
		destroy_bvh(mesh->bvh);
		if (mesh->materials) {
			for (int i = 0; i < mesh->materialCount; ++i) {
				destroyMaterial(&mesh->materials[i]);
			}
			free(mesh->materials);
		}
	}
}
