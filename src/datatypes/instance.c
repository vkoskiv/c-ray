//
//  instance.c
//  C-ray
//
//  Created by Valtteri on 23.6.2020.
//  Copyright © 2020 Valtteri Koskivuori. All rights reserved.
//

#include "../includes.h"
#include "../accelerators/bvh.h"
#include "../renderer/pathtrace.h"
#include "vector.h"
#include "instance.h"
#include "transforms.h"
#include "bbox.h"
#include "mesh.h"
#include "sphere.h"
#include "scene.h"

static bool intersectSphere(void *object, const struct lightRay *ray, struct hitRecord *isect) {
	if (rayIntersectsWithSphere(ray, (struct sphere*)object, isect)) {
		isect->polygon = NULL;
		isect->material = ((struct sphere*)object)->material;
		return true;
	}
	return false;
}

static void getSphereBBoxAndCenter(void *object, struct boundingBox *bbox, struct vector *center) {
	struct sphere *sphere = (struct sphere*)object;
	bbox->min = vecWithPos(-sphere->radius, -sphere->radius, -sphere->radius);
	bbox->max = vecWithPos( sphere->radius,  sphere->radius,  sphere->radius);
	*center = vecZero();
}

struct instance newSphereInstance(struct sphere *sphere) {
	return (struct instance) {
		.object = sphere,
		.composite = newTransform(),
		.intersectFn = intersectSphere,
		.getBBoxAndCenterFn = getSphereBBoxAndCenter
	};
}

static bool intersectMesh(void *object, const struct lightRay *ray, struct hitRecord *isect) {
	return traverseBottomLevelBvh((struct mesh*)object, ray, isect);
}

static void getMeshBBoxAndCenter(void *object, struct boundingBox *bbox, struct vector *center) {
	struct mesh *mesh = (struct mesh*)object;
	getRootBoundingBox(mesh->bvh, bbox);
	*center = bboxCenter(bbox);
}

struct instance newMeshInstance(struct mesh *mesh) {
	return (struct instance) {
		.object = mesh,
		.composite = newTransform(),
		.intersectFn = intersectMesh,
		.getBBoxAndCenterFn = getMeshBBoxAndCenter
	};
}

void addInstanceToScene(struct world *scene, struct instance instance) {
	if (scene->instanceCount == 0) {
		scene->instances = calloc(1, sizeof(*scene->instances));
	} else {
		scene->instances = realloc(scene->instances, (scene->instanceCount + 1) * sizeof(*scene->instances));
	}
	scene->instances[scene->instanceCount++] = instance;
}
