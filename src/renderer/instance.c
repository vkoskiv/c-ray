//
//  instance.c
//  C-ray
//
//  Created by Valtteri Koskivuori on 23.6.2020.
//  Copyright © 2020-2022 Valtteri Koskivuori. All rights reserved.
//

#include "../includes.h"
#include "../accelerators/bvh.h"
#include "../renderer/pathtrace.h"
#include "../datatypes/vector.h"
#include "instance.h"
#include "../datatypes/bbox.h"
#include "../datatypes/mesh.h"
#include "../datatypes/sphere.h"
#include "../datatypes/scene.h"
#include "../utils/args.h"

struct sphereVolume {
	struct sphere *sphere;
	float density;
};

struct meshVolume {
	struct mesh *mesh;
	float density;
};

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

static bool intersectSphere(const struct instance *instance, const struct lightRay *ray, struct hitRecord *isect, sampler *sampler) {
	(void)sampler;
	struct lightRay copy = *ray;
	transformRay(&copy, instance->composite.Ainv);
	struct sphere *sphere = (struct sphere *)instance->object;
	copy.start = vecAdd(copy.start, vecScale(copy.direction, sphere->rayOffset));
	if (rayIntersectsWithSphere(&copy, sphere, isect)) {
		isect->uv = getTexMapSphere(isect);
		isect->polygon = NULL;
		isect->bsdf = sphere->material.bsdf;
		isect->emission = &sphere->material.emission;
		transformPoint(&isect->hitPoint, instance->composite.A);
		transformVectorWithTranspose(&isect->surfaceNormal, instance->composite.Ainv);
		return true;
	}
	return false;
}

static bool intersectSphereVolume(const struct instance *instance, const struct lightRay *ray, struct hitRecord *isect, sampler *sampler) {
	struct hitRecord record1, record2;
	record1 = *isect;
	record2 = *isect;
	struct lightRay copy1, copy2;
	copy1 = *ray;
	transformRay(&copy1, instance->composite.Ainv);
	struct sphereVolume *volume = (struct sphereVolume *)instance->object;
	copy1.start = vecAdd(copy1.start, vecScale(copy1.direction, volume->sphere->rayOffset));
	if (rayIntersectsWithSphere(&copy1, volume->sphere, &record1)) {
		copy2 = (struct lightRay){ alongRay(&copy1, record1.distance + 0.0001f), copy1.direction };
		if (rayIntersectsWithSphere(&copy2, volume->sphere, &record2)) {
			if (record1.distance < 0.0f)
				record1.distance = 0.0f;
			float distanceInsideVolume = record2.distance;
			float hitDistance = -(1.0f / volume->density) * logf(getDimension(sampler));
			if (hitDistance < distanceInsideVolume) {
				isect->distance = record1.distance + hitDistance;
				isect->hitPoint = alongRay(ray, isect->distance);
				isect->uv = (struct coord){-1.0f, -1.0f};
				isect->polygon = NULL;
				isect->bsdf = volume->sphere->material.bsdf;
				isect->emission = &volume->sphere->material.emission;
				transformPoint(&isect->hitPoint, instance->composite.A);
				isect->surfaceNormal = (struct vector){1.0f, 0.0f, 0.0f}; // Will be ignored by material anyway
				transformVectorWithTranspose(&isect->surfaceNormal, instance->composite.Ainv); // Probably not needed
				return true;
			}
		}
	}
	return false;
}

static void getSphereBBoxAndCenter(const struct instance *instance, struct boundingBox *bbox, struct vector *center) {
	struct sphere *sphere = (struct sphere *)instance->object;
	*center = vecZero();
	transformPoint(center, instance->composite.A);
	bbox->min = (struct vector){ -sphere->radius, -sphere->radius, -sphere->radius };
	bbox->max = (struct vector){  sphere->radius,  sphere->radius,  sphere->radius };
	if (!isRotation(&instance->composite) && !isTranslate(&instance->composite))
		transformBBox(bbox, instance->composite.A);
	else {
		bbox->min = vecAdd(bbox->min, *center);
		bbox->max = vecAdd(bbox->max, *center);
	}
	sphere->rayOffset = rayOffset(*bbox);
	if (isSet("v")) logr(plain, "\n");
	logr(debug, "sphere offset: %f", sphere->rayOffset);
}

static void getSphereVolumeBBoxAndCenter(const struct instance *instance, struct boundingBox *bbox, struct vector *center) {
	struct sphereVolume *volume = (struct sphereVolume *)instance->object;
	*center = vecZero();
	transformPoint(center, instance->composite.A);
	bbox->min = (struct vector){ -volume->sphere->radius, -volume->sphere->radius, -volume->sphere->radius };
	bbox->max = (struct vector){  volume->sphere->radius,  volume->sphere->radius,  volume->sphere->radius };
	if (!isRotation(&instance->composite) && !isTranslate(&instance->composite))
		transformBBox(bbox, instance->composite.A);
	else {
		bbox->min = vecAdd(bbox->min, *center);
		bbox->max = vecAdd(bbox->max, *center);
	}
	volume->sphere->rayOffset = rayOffset(*bbox);
	if (isSet("v")) logr(plain, "\n");
	logr(debug, "sphere offset: %f", volume->sphere->rayOffset);
}

struct instance newSphereSolid(struct sphere *sphere) {
	return (struct instance) {
		.object = sphere,
		.composite = newTransform(),
		.intersectFn = intersectSphere,
		.getBBoxAndCenterFn = getSphereBBoxAndCenter
	};
}

struct instance newSphereVolume(struct sphere *sphere, float density, struct block **pool) {
	struct sphereVolume *volume = allocBlock(pool, sizeof(*volume));
	volume->sphere = sphere;
	volume->density = density;
	return (struct instance) {
		.object = volume,
		.composite = newTransform(),
		.intersectFn = intersectSphereVolume,
		.getBBoxAndCenterFn = getSphereVolumeBBoxAndCenter
	};
}

static struct coord getTexMapMesh(const struct mesh *mesh, const struct hitRecord *isect) {
	if (mesh->tex_coord_count == 0) return (struct coord){-1.0f, -1.0f};
	struct poly *p = isect->polygon;
	if (p->textureIndex[0] == -1) return (struct coord){-1.0f, -1.0f};
	
	//barycentric coordinates for this polygon
	const float u = isect->uv.x;
	const float v = isect->uv.y;
	const float w = 1.0f - u - v;
	
	//Weighted texture coordinates
	const struct coord ucomponent = coordScale(u, mesh->texture_coords[p->textureIndex[1]]);
	const struct coord vcomponent = coordScale(v, mesh->texture_coords[p->textureIndex[2]]);
	const struct coord wcomponent = coordScale(w, mesh->texture_coords[p->textureIndex[0]]);
	
	// textureXY = u * v1tex + v * v2tex + w * v3tex
	return addCoords(addCoords(ucomponent, vcomponent), wcomponent);
}

static bool intersectMesh(const struct instance *instance, const struct lightRay *ray, struct hitRecord *isect, sampler *sampler) {
	struct lightRay copy = *ray;
	transformRay(&copy, instance->composite.Ainv);
	struct mesh *mesh = instance->object;
	copy.start = vecAdd(copy.start, vecScale(copy.direction, mesh->rayOffset));
	if (traverseBottomLevelBvh(mesh, &copy, isect, sampler)) {
		// Repopulate uv with actual texture mapping
		isect->uv = getTexMapMesh(mesh, isect);
		isect->bsdf = instance->materials[isect->polygon->materialIndex].bsdf;
		isect->emission = &instance->materials[isect->polygon->materialIndex].emission;
		transformPoint(&isect->hitPoint, instance->composite.A);
		transformVectorWithTranspose(&isect->surfaceNormal, instance->composite.Ainv);
		isect->surfaceNormal = vecNormalize(isect->surfaceNormal);
		return true;
	}
	return false;
}

static bool intersectMeshVolume(const struct instance *instance, const struct lightRay *ray, struct hitRecord *isect, sampler *sampler) {
	struct hitRecord record1, record2;
	record1 = *isect;
	record2 = *isect;
	struct lightRay copy = *ray;
	transformRay(&copy, instance->composite.Ainv);
	struct meshVolume *mesh = (struct meshVolume *)instance->object;
	copy.start = vecAdd(copy.start, vecScale(copy.direction, mesh->mesh->rayOffset));
	if (traverseBottomLevelBvh(mesh->mesh, &copy, &record1, sampler)) {
		struct lightRay copy2 = (struct lightRay){ alongRay(&copy, record1.distance + 0.0001f), copy.direction };
		if (traverseBottomLevelBvh(mesh->mesh, &copy2, &record2, sampler)) {
			if (record1.distance < 0.0f)
				record1.distance = 0.0f;
			float distanceInsideVolume = record2.distance;
			float hitDistance = -(1.0f / mesh->density) * logf(getDimension(sampler));
			if (hitDistance < distanceInsideVolume) {
				isect->distance = record1.distance + hitDistance;
				isect->hitPoint = alongRay(ray, isect->distance);
				isect->uv = (struct coord){-1.0f, -1.0f};
				isect->bsdf = mesh->mesh->materials[0].bsdf;
				isect->emission = &mesh->mesh->materials[0].emission;
				transformPoint(&isect->hitPoint, instance->composite.A);
				isect->surfaceNormal = (struct vector){1.0f, 0.0f, 0.0f}; // Will be ignored by material anyway
				transformVectorWithTranspose(&isect->surfaceNormal, instance->composite.Ainv); // Probably not needed
				return true;
			}
		}
	}
	return false;
}

bool isMesh(const struct instance *instance) {
	return instance->intersectFn == intersectMesh;
}

static void getMeshBBoxAndCenter(const struct instance *instance, struct boundingBox *bbox, struct vector *center) {
	struct mesh *mesh = (struct mesh *)instance->object;
	*bbox = getRootBoundingBox(mesh->bvh);
	transformBBox(bbox, instance->composite.A);
	*center = bboxCenter(bbox);
	mesh->rayOffset = rayOffset(*bbox);
	if (isSet("v")) logr(plain, "\n");
	logr(debug, "mesh \"%s\" offset: %f", mesh->name, mesh->rayOffset);
}

static void getMeshVolumeBBoxAndCenter(const struct instance *instance, struct boundingBox *bbox, struct vector *center) {
	struct meshVolume *volume = (struct meshVolume *)instance->object;
	*bbox = getRootBoundingBox(volume->mesh->bvh);
	transformBBox(bbox, instance->composite.A);
	*center = bboxCenter(bbox);
	volume->mesh->rayOffset = rayOffset(*bbox);
	if (isSet("v")) logr(plain, "\n");
	logr(debug, "mesh \"%s\" offset: %f", volume->mesh->name, volume->mesh->rayOffset);
}

struct instance newMeshSolid(struct mesh *mesh) {
	return (struct instance) {
		.object = mesh,
		.composite = newTransform(),
		.intersectFn = intersectMesh,
		.getBBoxAndCenterFn = getMeshBBoxAndCenter
	};
}

struct instance newMeshVolume(struct mesh *mesh, float density, struct block **pool) {
	struct meshVolume *volume = allocBlock(pool, sizeof(*volume));
	volume->mesh = mesh;
	volume->density = density;
	return (struct instance) {
		.object = volume,
		.composite = newTransform(),
		.intersectFn = intersectMeshVolume,
		.getBBoxAndCenterFn = getMeshVolumeBBoxAndCenter
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
