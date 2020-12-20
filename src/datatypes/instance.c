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
#include "../utils/args.h"
#include "../datatypes/vertexbuffer.h"

static inline struct coord getTexMapSphere(const struct hitRecord *isect) {
	struct vector ud = isect->surfaceNormal;
	//To polar from cartesian
	float phi = atan2f(ud.z, ud.x);
	float theta = asinf(ud.y);
	float v = (theta + PI / 2.0f) / PI;
	float u = 1.0f - (phi + PI) / (PI * 2.0f);
	u = wrapMinMax(u, 0.0f, 1.0f);
	v = wrapMinMax(v, 0.0f, 1.0f);
	return (struct coord){ u, v };
}

static bool intersectSphere(const struct instance *instance, const struct lightRay *ray, struct hitRecord *isect) {
	struct lightRay copy = *ray;
	transformRay(&copy, &instance->composite.Ainv);
	struct sphere *sphere = (struct sphere*)instance->object;
	float offset = sphere->rayOffset;
	copy.start = vecAdd(copy.start, vecScale(copy.direction, offset));
	if (rayIntersectsWithSphere(&copy, sphere, isect)) {
		isect->uv = getTexMapSphere(isect);
		isect->polygon = NULL;
		isect->material = sphere->material;
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
	if (isSet("v")) printf("\n");
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

static struct coord getTexMapMesh(const struct mesh *mesh, const struct hitRecord *isect) {
	if (mesh->textureCoordCount == 0) return (struct coord){-1.0f, -1.0f};
	struct poly *p = isect->polygon;
	
	//barycentric coordinates for this polygon
	const float u = isect->uv.x;
	const float v = isect->uv.y;
	const float w = 1.0f - u - v;
	
	//Weighted texture coordinates
	const struct coord ucomponent = coordScale(u, g_textureCoords[p->textureIndex[1]]);
	const struct coord vcomponent = coordScale(v, g_textureCoords[p->textureIndex[2]]);
	const struct coord wcomponent = coordScale(w, g_textureCoords[p->textureIndex[0]]);
	
	// textureXY = u * v1tex + v * v2tex + w * v3tex
	return addCoords(addCoords(ucomponent, vcomponent), wcomponent);
}

static bool intersectMesh(const struct instance *instance, const struct lightRay *ray, struct hitRecord *isect) {
	struct lightRay copy = *ray;
	transformRay(&copy, &instance->composite.Ainv);
	struct mesh *mesh = (struct mesh *)instance->object;
	float offset = mesh->rayOffset;
	copy.start = vecAdd(copy.start, vecScale(copy.direction, offset));
	if (traverseBottomLevelBvh(mesh, &copy, isect)) {
		// Repopulate uv with actual texture mapping
		isect->uv = getTexMapMesh(mesh, isect);
		isect->material = mesh->materials[isect->polygon->materialIndex];
		transformPoint(&isect->hitPoint, &instance->composite.A);
		transformVectorWithTranspose(&isect->surfaceNormal, &instance->composite.Ainv);
		isect->surfaceNormal = vecNormalize(isect->surfaceNormal);
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
	if (isSet("v")) printf("\n");
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
