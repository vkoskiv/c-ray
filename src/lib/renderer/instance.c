//
//  instance.c
//  c-ray
//
//  Created by Valtteri Koskivuori on 23.6.2020.
//  Copyright © 2020-2025 Valtteri Koskivuori. All rights reserved.
//

#include "../../includes.h"
#include <common/vector.h>
#include <accelerators/bvh.h>
#include <renderer/pathtrace.h>
#include "instance.h"
#include <datatypes/bbox.h>
#include <datatypes/mesh.h>
#include <datatypes/sphere.h>
#include <datatypes/scene.h>

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
	u = wrap_min_max(u, 0.0f, 1.0f);
	v = wrap_min_max(v, 0.0f, 1.0f);
	return (struct coord){ u, v };
}

static bool intersectSphere(const struct instance *instance, const struct lightRay *ray, struct hitRecord *isect, sampler *sampler) {
	(void)sampler;
	struct lightRay copy = *ray;
	tform_ray(&copy, instance->composite.Ainv);
	struct sphere *sphere = &((struct sphere_arr *)instance->object_arr)->items[instance->object_idx];
	copy.start = vec_add(copy.start, vec_scale(copy.direction, sphere->rayOffset));
	if (rayIntersectsWithSphere(&copy, sphere, isect)) {
		isect->uv = getTexMapSphere(isect);
		isect->polygon = NULL;
		isect->bsdf = instance->bbuf->bsdfs.items[0];
		tform_point(&isect->hitPoint, instance->composite.A);
		tform_vector_transpose(&isect->surfaceNormal, instance->composite.Ainv);
		return true;
	}
	return false;
}

static bool intersectSphereVolume(const struct instance *instance, const struct lightRay *ray, struct hitRecord *isect, sampler *sampler) {
	return false;
	struct hitRecord record1, record2;
	record1 = *isect;
	record2 = *isect;
	struct lightRay copy1, copy2;
	copy1 = *ray;
	tform_ray(&copy1, instance->composite.Ainv);
	//FIXME
	struct sphereVolume *volume = NULL;//(struct sphereVolume *)instance->object;
	copy1.start = vec_add(copy1.start, vec_scale(copy1.direction, volume->sphere->rayOffset));
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
				isect->bsdf = instance->bbuf->bsdfs.items[0];
				tform_point(&isect->hitPoint, instance->composite.A);
				isect->surfaceNormal = (struct vector){1.0f, 0.0f, 0.0f}; // Will be ignored by material anyway
				tform_vector_transpose(&isect->surfaceNormal, instance->composite.Ainv); // Probably not needed
				return true;
			}
		}
	}
	return false;
}

static void getSphereBBoxAndCenter(const struct instance *instance, struct boundingBox *bbox, struct vector *center) {
	struct sphere *sphere = &((struct sphere_arr *)instance->object_arr)->items[instance->object_idx];
	bbox->min = (struct vector){ -sphere->radius, -sphere->radius, -sphere->radius };
	bbox->max = (struct vector){  sphere->radius,  sphere->radius,  sphere->radius };
	tform_bbox(bbox, instance->composite.A);
	*center = bboxCenter(bbox);
	sphere->rayOffset = rayOffset(*bbox);
}

static void getSphereVolumeBBoxAndCenter(const struct instance *instance, struct boundingBox *bbox, struct vector *center) {
	return;
	struct sphereVolume *volume = NULL;//(struct sphereVolume *)instance->object;
	*center = vec_zero();
	tform_point(center, instance->composite.A);
	bbox->min = (struct vector){ -volume->sphere->radius, -volume->sphere->radius, -volume->sphere->radius };
	bbox->max = (struct vector){  volume->sphere->radius,  volume->sphere->radius,  volume->sphere->radius };
	bbox->min = vec_add(bbox->min, *center);
	bbox->max = vec_add(bbox->max, *center);
	volume->sphere->rayOffset = rayOffset(*bbox);
}

struct instance new_sphere_instance(struct sphere_arr *spheres, size_t idx, float *density, struct block **pool) {
	if (density && pool) {
		struct sphereVolume *volume = allocBlock(pool, sizeof(*volume));
		volume->sphere = NULL;//sphere;
		volume->density = *density;
		return (struct instance) {
			.object_arr = NULL,
			.object_idx = 0,
			.composite = tform_new(),
			.intersectFn = intersectSphereVolume,
			.getBBoxAndCenterFn = getSphereVolumeBBoxAndCenter
		};
	} else {
		return (struct instance) {
			.object_arr = spheres,
			.object_idx = idx,
			.composite = tform_new(),
			.intersectFn = intersectSphere,
			.getBBoxAndCenterFn = getSphereBBoxAndCenter
		};
	}
}

static struct coord getTexMapMesh(const struct mesh *mesh, const struct hitRecord *isect) {
	if (mesh->vbuf.texture_coords.count == 0) return (struct coord){-1.0f, -1.0f};
	struct poly *p = isect->polygon;
	if (p->textureIndex[0] == -1) return (struct coord){-1.0f, -1.0f};
	
	//barycentric coordinates for this polygon
	const float u = isect->uv.x;
	const float v = isect->uv.y;
	const float w = 1.0f - u - v;
	
	//Weighted texture coordinates
	const struct coord ucomponent = coord_scale(u, mesh->vbuf.texture_coords.items[p->textureIndex[1]]);
	const struct coord vcomponent = coord_scale(v, mesh->vbuf.texture_coords.items[p->textureIndex[2]]);
	const struct coord wcomponent = coord_scale(w, mesh->vbuf.texture_coords.items[p->textureIndex[0]]);
	
	// textureXY = u * v1tex + v * v2tex + w * v3tex
	return coord_add(coord_add(ucomponent, vcomponent), wcomponent);
}

static bool intersectMesh(const struct instance *instance, const struct lightRay *ray, struct hitRecord *isect, sampler *sampler) {
	struct lightRay copy = *ray;
	tform_ray(&copy, instance->composite.Ainv);
	struct mesh *mesh = &((struct mesh_arr *)instance->object_arr)->items[instance->object_idx];
	if (!mesh->bvh) return false;
	copy.start = vec_add(copy.start, vec_scale(copy.direction, mesh->rayOffset));
	if (traverse_bottom_level_bvh(mesh, &copy, isect, sampler)) {
		// Repopulate uv with actual texture mapping
		isect->uv = getTexMapMesh(mesh, isect);
		isect->bsdf = instance->bbuf->bsdfs.items[isect->polygon->materialIndex];
		tform_point(&isect->hitPoint, instance->composite.A);
		tform_vector_transpose(&isect->surfaceNormal, instance->composite.Ainv);
		isect->surfaceNormal = vec_normalize(isect->surfaceNormal);
		return true;
	}
	return false;
}

static bool intersectMeshVolume(const struct instance *instance, const struct lightRay *ray, struct hitRecord *isect, sampler *sampler) {
	return false;
	struct hitRecord record1, record2;
	record1 = *isect;
	record2 = *isect;
	struct lightRay copy = *ray;
	tform_ray(&copy, instance->composite.Ainv);
	//FIXME
	struct meshVolume *mesh = NULL;//(struct meshVolume *)instance->object;
	copy.start = vec_add(copy.start, vec_scale(copy.direction, mesh->mesh->rayOffset));
	if (traverse_bottom_level_bvh(mesh->mesh, &copy, &record1, sampler)) {
		struct lightRay copy2 = (struct lightRay){ alongRay(&copy, record1.distance + 0.0001f), copy.direction };
		if (traverse_bottom_level_bvh(mesh->mesh, &copy2, &record2, sampler)) {
			if (record1.distance < 0.0f)
				record1.distance = 0.0f;
			float distanceInsideVolume = record2.distance;
			float hitDistance = -(1.0f / mesh->density) * logf(getDimension(sampler));
			if (hitDistance < distanceInsideVolume) {
				isect->distance = record1.distance + hitDistance;
				isect->hitPoint = alongRay(ray, isect->distance);
				isect->uv = (struct coord){-1.0f, -1.0f};
				isect->bsdf = instance->bbuf->bsdfs.items[0];
				tform_point(&isect->hitPoint, instance->composite.A);
				isect->surfaceNormal = (struct vector){1.0f, 0.0f, 0.0f}; // Will be ignored by material anyway
				tform_vector_transpose(&isect->surfaceNormal, instance->composite.Ainv); // Probably not needed
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
	struct mesh *mesh = &((struct mesh_arr *)instance->object_arr)->items[instance->object_idx];
	if (!mesh->bvh) {
		*bbox = (struct boundingBox){ 0 };
		*center = vec_zero();
		mesh->rayOffset = 0.0f;
		return;
	}
	*bbox = get_transformed_root_bbox(mesh->bvh, &instance->composite.A);
	*center = bboxCenter(bbox);
	mesh->rayOffset = rayOffset(*bbox);
}

static void getMeshVolumeBBoxAndCenter(const struct instance *instance, struct boundingBox *bbox, struct vector *center) {
	return;
	//FIXME
	struct meshVolume *volume = NULL;//(struct meshVolume *)instance->object;
	*bbox = get_root_bbox(volume->mesh->bvh);
	tform_bbox(bbox, instance->composite.A);
	*center = bboxCenter(bbox);
	volume->mesh->rayOffset = rayOffset(*bbox);
}

struct instance new_mesh_instance(struct mesh_arr *meshes, size_t idx, float *density, struct block **pool) {
	if (density && pool) {
		struct meshVolume *volume = allocBlock(pool, sizeof(*volume));
		volume->mesh = NULL;//mesh;
		volume->density = *density;
		return (struct instance) {
			//FIXME
			.object_arr = NULL,
			.object_idx = 0,
			.composite = tform_new(),
			.intersectFn = intersectMeshVolume,
			.getBBoxAndCenterFn = getMeshVolumeBBoxAndCenter
		};
	} else {
		return (struct instance) {
			.object_arr = meshes,
			.object_idx = idx,
			.composite = tform_new(),
			.intersectFn = intersectMesh,
			.getBBoxAndCenterFn = getMeshBBoxAndCenter
		};
	}
}
