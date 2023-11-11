//
//  instance.h
//  C-ray
//
//  Created by Valtteri Koskivuori on 23.6.2020.
//  Copyright © 2020-2022 Valtteri Koskivuori. All rights reserved.
//

#pragma once

#include <stdbool.h>
#include <stddef.h>
#include "../datatypes/transforms.h"
#include "../utils/mempool.h"
#include "../utils/dyn_array.h"
#include "samplers/sampler.h"
#include "../nodes/bsdfnode.h"
#include "../datatypes/mesh.h"
#include "../datatypes/sphere.h"

struct lightRay;
struct hitRecord;

struct instance {
	struct transform composite;
	struct bsdf_buffer *bbuf;
	bool emits_light;
	bool (*intersectFn)(const struct instance *, const struct lightRay *, struct hitRecord *, sampler *);
	void (*getBBoxAndCenterFn)(const struct instance *, struct boundingBox *, struct vector *);
	void *object_arr;
	size_t object_idx;
};

typedef struct instance instance;
dyn_array_def(instance);

struct instance new_sphere_instance(struct sphere_arr *spheres, size_t idx, float *density, struct block **pool);
struct instance new_mesh_instance(struct mesh_arr *meshes, size_t idx, float *density, struct block **pool);

bool isMesh(const struct instance *instance);
