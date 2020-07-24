//
//  mesh.c
//  C-ray
//
//  Created by Valtteri Koskivuori on 27/04/2017.
//  Copyright © 2015-2020 Valtteri Koskivuori. All rights reserved.
//

#include "../includes.h"
#include "mesh.h"

#include "../accelerators/bvh.h"
#include "vertexbuffer.h"
#include "transforms.h"
#include "poly.h"
#include "material.h"
#include "vector.h"

void destroyMesh(struct mesh *mesh) {
	if (mesh) {
		free(mesh->name);
		free(mesh->polygons);
		destroyBvh(mesh->bvh);
		if (mesh->materials) {
			for (int i = 0; i < mesh->materialCount; ++i) {
				destroyMaterial(&mesh->materials[i]);
			}
			free(mesh->materials);
		}
	}
}
