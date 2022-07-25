//
//  instance.h
//  C-ray
//
//  Created by Valtteri Koskivuori on 23.6.2020.
//  Copyright © 2020-2022 Valtteri Koskivuori. All rights reserved.
//

#pragma once

#include <stdbool.h>
#include "../datatypes/transforms.h"
#include "samplers/sampler.h"

struct world;

struct lightRay;
struct hitRecord;

struct sphere;
struct mesh;
struct material;

struct instance {
	struct transform composite;
	struct material *materials; //FIXME: Make this an array of bsdf ptrs instead
	size_t material_count;
	bool (*intersectFn)(const struct instance *, const struct lightRay *, struct hitRecord *, sampler *);
	void (*getBBoxAndCenterFn)(const struct instance *, struct boundingBox *, struct vector *);
	void *object;
};

struct instance newSphereSolid(struct sphere *sphere);
struct instance newSphereVolume(struct sphere *sphere, float density, struct block **pool);
struct instance newMeshSolid(struct mesh *mesh);
struct instance newMeshVolume(struct mesh *mesh, float density, struct block **pool);

bool isMesh(const struct instance *instance);

void addInstanceToScene(struct world *scene, struct instance instance);
