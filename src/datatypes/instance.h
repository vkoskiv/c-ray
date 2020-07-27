//
//  instance.h
//  C-ray
//
//  Created by Valtteri on 23.6.2020.
//  Copyright © 2020 Valtteri Koskivuori. All rights reserved.
//

#pragma once

#include <stdbool.h>
#include "transforms.h"

struct world;
struct matrix4x4;
struct lightRay;
struct hitRecord;

struct sphere;
struct mesh;

struct instance {
	enum {Mesh, Sphere} type;
	struct transform composite;
	bool (*intersectFn)(const struct instance*, const struct lightRay*, struct hitRecord*);
	void (*getBBoxAndCenterFn)(const struct instance*, struct boundingBox*, struct vector*);
	void *object;
};

struct instance newSphereInstance(struct sphere *sphere);
struct instance newMeshInstance(struct mesh *mesh);

void addInstanceToScene(struct world *scene, struct instance instance);
