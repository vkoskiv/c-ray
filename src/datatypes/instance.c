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
#include "../utils/logging.h"

static bool intersectSphere(const struct instance *instance, const struct lightRay *ray, struct hitRecord *isect) {
	struct lightRay copy = *ray;
	transformRay(&copy, &instance->composite.Ainv);
	float offset = ((struct sphere*)instance->object)->rayOffset;
	copy.start = vecAdd(copy.start, vecScale(copy.direction, offset));
	if (rayIntersectsWithSphere(&copy, (struct sphere*)instance->object, isect)) {
		isect->polygon = NULL;
		isect->material = ((struct sphere*)instance->object)->material;
		transformPoint(&isect->hitPoint, &instance->composite.A);
		transformVectorWithTranspose(&isect->surfaceNormal, &instance->composite.Ainv);
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
	sphere->rayOffset = rayOffset(*bbox);
	printf("\n");
	logr(debug, "sphere offset: %f", sphere->rayOffset);
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
	float offset = ((struct mesh*)instance->object)->rayOffset;
	copy.start = vecAdd(copy.start, vecScale(copy.direction, offset));
	if (traverseBottomLevelBvh((struct mesh*)instance->object, &copy, isect)) {
		isect->material = ((struct mesh*)instance->object)->materials[isect->polygon->materialIndex];
		transformPoint(&isect->hitPoint, &instance->composite.A);
		transformVectorWithTranspose(&isect->surfaceNormal, &instance->composite.Ainv);
		if (likely(!isect->material.hasNormalMap)) {
			isect->surfaceNormal = vecNormalize(isect->surfaceNormal);
		} else {
			//struct color pixel = colorForUV(isect, Normal);
			// FIXME
			//isect->surfaceNormal = vecNormalize((struct vector){(pixel.red * 2.0f) - 1.0f, (pixel.green * 2.0f) - 1.0f, pixel.blue * 0.5f});
			isect->surfaceNormal = vecNormalize(isect->surfaceNormal);
		}
		return true;
	}
	return false;
}

static void getMeshBBoxAndCenter(const struct instance *instance, struct boundingBox *bbox, struct vector *center) {
	struct mesh *mesh = (struct mesh*)instance->object;
	*bbox = getRootBoundingBox(mesh->bvh);
	transformBBox(bbox, &instance->composite.A);
	*center = bboxCenter(bbox);
	mesh->rayOffset = rayOffset(*bbox);
	printf("\n");
	logr(debug, "mesh \"%s\" offset: %f", mesh->name, mesh->rayOffset);
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
