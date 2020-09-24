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

static bool intersectSphere(const struct instance *instance, const struct lightRay *ray, struct hitRecord *isect) {
	struct lightRay copy = *ray;
	transformRay(&copy, &instance->composite.Ainv);

	if (rayIntersectsWithSphere(&copy, (struct sphere*)instance->object, isect)) {
		isect->polygon = NULL;
		isect->material = ((struct sphere*)instance->object)->material;
		return true;
	}
	return false;
}

static void getSphereBBoxAndCenter(const struct instance *instance, struct boundingBox *bbox, struct vector *center) {
	struct sphere *sphere = (struct sphere*)instance->object;
	*center = vecZero();
	transformPoint(center, &instance->composite.A);
	bbox->min = vecWithPos(-sphere->radius, -sphere->radius, -sphere->radius);
	bbox->max = vecWithPos( sphere->radius,  sphere->radius,  sphere->radius);
	if (!isRotation(&instance->composite) && !isTranslate(&instance->composite))
		transformBBox(bbox, &instance->composite.A);
	else {
		bbox->min = vecAdd(bbox->min, *center);
		bbox->max = vecAdd(bbox->max, *center);
	}
}

struct instance newSphereInstance(struct sphere *sphere) {
	return (struct instance) {
		.type = Sphere,
		.object = sphere,
		.composite = newTransform(),
		.intersectFn = intersectSphere,
		.getBBoxAndCenterFn = getSphereBBoxAndCenter
	};
}

static bool intersectMesh(const struct instance *instance, const struct lightRay *ray, struct hitRecord *isect) {
	struct lightRay copy = *ray;
	transformRay(&copy, &instance->composite.Ainv);
	return traverseBottomLevelBvh((struct mesh*)instance->object, &copy, isect);
}

static void getMeshBBoxAndCenter(const struct instance *instance, struct boundingBox *bbox, struct vector *center) {
	*bbox = getRootBoundingBox(((struct mesh*)instance->object)->bvh);
	transformBBox(bbox, &instance->composite.A);
	*center = bboxCenter(bbox);
}

struct instance newMeshInstance(struct mesh *mesh) {
	return (struct instance) {
		.type = Mesh,
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
