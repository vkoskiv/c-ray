//
//  instance.h
//  c-ray
//
//  Created by Valtteri Koskivuori on 23.6.2020.
//  Copyright © 2020-2025 Valtteri Koskivuori. All rights reserved.
//

#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <v.h>

#include <common/transforms.h>
#include <common/mempool.h>
#include "samplers/sampler.h"
#include <nodes/bsdfnode.h>
#include <datatypes/mesh.h>
#include <datatypes/sphere.h>

struct lightRay;
struct hitRecord;

struct instance {
	struct transform composite;
	struct bsdf_buffer *bbuf;
	size_t bbuf_idx;
	bool emits_light;
	bool (*intersectFn)(const struct instance *, const struct lightRay *, struct hitRecord *, sampler *);
	void (*getBBoxAndCenterFn)(const struct instance *, struct boundingBox *, struct vector *);
	void *object_arr;
	size_t object_idx;
};

typedef struct instance instance;
v_arr_def(instance)

struct instance new_sphere_instance(struct sphere_arr *spheres, size_t idx, float *density, struct block **pool);
struct instance new_mesh_instance(struct mesh_arr *meshes, size_t idx, float *density, struct block **pool);

enum cr_instance_type {
	CR_I_UNKNOWN = 0,
	CR_I_MESH,
	CR_I_SPHERE,
};

enum cr_instance_type instance_type(const struct instance *i);
