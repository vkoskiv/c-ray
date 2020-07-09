//
//  pathtrace.c
//  C-ray
//
//  Created by Valtteri Koskivuori on 27/04/2017.
//  Copyright © 2015-2020 Valtteri Koskivuori. All rights reserved.
//

#include "../includes.h"
#include "pathtrace.h"

#include "../datatypes/scene.h"
#include "../datatypes/camera.h"
#include "../accelerators/bvh.h"
#include "../datatypes/image/texture.h"
#include "../datatypes/image/hdr.h"
#include "../datatypes/vertexbuffer.h"
#include "../datatypes/sphere.h"
#include "../datatypes/poly.h"
#include "../datatypes/mesh.h"
#include "samplers/sampler.h"
#include "sky.h"
#include "../datatypes/transforms.h"
#include "../datatypes/instance.h"

#define LINEAR

static struct hitRecord getClosestIsect(const struct lightRay *incidentRay, const struct world *scene);
static struct color getBackground(const struct lightRay *incidentRay, const struct world *scene);

struct color debugNormals(const struct lightRay *incidentRay, const struct world *scene, int maxDepth, sampler *sampler) {
	(void)maxDepth;
	(void)sampler;
	struct lightRay currentRay = *incidentRay;
	struct hitRecord isect = getClosestIsect(&currentRay, scene);
	if (isect.type == hitTypeNone) return getBackground(&currentRay, scene);
	struct vector normal = vecNormalize(isect.surfaceNormal);
	return colorWithValues(fabs(normal.x), fabs(normal.y), fabs(normal.z), 1.0f);
}

struct color pathTrace(const struct lightRay *incidentRay, const struct world *scene, int maxDepth, sampler *sampler) {
	struct color weight = whiteColor; // Current path weight
	struct color finalColor = blackColor; // Final path contribution
	struct lightRay currentRay = *incidentRay;

	for (int depth = 0; depth < maxDepth; ++depth) {
		struct hitRecord isect = getClosestIsect(&currentRay, scene);
		if (isect.type == hitTypeNone) {
			finalColor = addColors(finalColor, multiplyColors(weight, getBackground(&currentRay, scene)));
			break;
		}

		finalColor = addColors(finalColor, multiplyColors(weight, isect.end.emission));
		
		struct color attenuation;
		if (!isect.end.bsdf(&isect, &attenuation, &currentRay, sampler))
			break;
		
		if (isect.type == hitTypePolygon) {
			transformVector(&currentRay.start, scene->instances[isect.instIndex].composite.A);
			transformDirection(&currentRay.direction, transpose(scene->instances[isect.instIndex].composite.Ainv));
		} else if (isect.type == hitTypeSphere) {
			transformVector(&currentRay.start, scene->sphereInstances[isect.instIndex].composite.A);
			transformDirection(&currentRay.direction, transpose(scene->sphereInstances[isect.instIndex].composite.Ainv));
		}
		
		float probability = 1.0f;
		if (depth >= 4) {
			probability = max(attenuation.red, max(attenuation.green, attenuation.blue));
			if (getDimension(sampler) > probability)
				break;
		}

		weight = colorCoef(1.0f / probability, multiplyColors(attenuation, weight));
	}
	return finalColor;
}

static inline void computeSurfaceProps(const struct poly *p, const struct coord *uv, struct vector *hitPoint, struct vector *normal) {
	float u = uv->x;
	float v = uv->y;
	float w = 1.0f - u - v;
	struct vector ucomp = vecScale(g_vertices[p->vertexIndex[2]], u);
	struct vector vcomp = vecScale(g_vertices[p->vertexIndex[1]], v);
	struct vector wcomp = vecScale(g_vertices[p->vertexIndex[0]], w);
	
	*hitPoint = vecAdd(vecAdd(ucomp, vcomp), wcomp);
	
	if (p->hasNormals) {
		struct vector upcomp = vecScale(g_normals[p->normalIndex[2]], u);
		struct vector vpcomp = vecScale(g_normals[p->normalIndex[1]], v);
		struct vector wpcomp = vecScale(g_normals[p->normalIndex[0]], w);
		
		*normal = vecNormalize(vecAdd(vecAdd(upcomp, vpcomp), wpcomp));
	}
	
	*hitPoint = vecAdd(*hitPoint, vecScale(*normal, 0.0001f));
}

static struct vector bumpmap(const struct hitRecord *isect) {
	struct material mtl = isect->end;
	struct poly *p = isect->polygon;
	float width = mtl.normalMap->width;
	float heigh = mtl.normalMap->height;
	float u = isect->uv.x;
	float v = isect->uv.y;
	float w = 1.0f - u - v;
	struct coord ucomponent = coordScale(u, g_textureCoords[p->textureIndex[2]]);
	struct coord vcomponent = coordScale(v, g_textureCoords[p->textureIndex[1]]);
	struct coord wcomponent = coordScale(w, g_textureCoords[p->textureIndex[0]]);
	struct coord textureXY = addCoords(addCoords(ucomponent, vcomponent), wcomponent);
	float x = (textureXY.x*(width));
	float y = (textureXY.y*(heigh));
	struct color pixel = textureGetPixelFiltered(mtl.normalMap, x, y);
	return vecNormalize((struct vector){(pixel.red * 2.0f) - 1.0f, (pixel.green * 2.0f) - 1.0f, pixel.blue * 0.5f});
}

/**
 Calculate the closest intersection point, and other relevant information based on a given lightRay and scene
 See the intersection struct for documentation of what this function calculates.

 @param incidentRay Given light ray (set up in renderThread())
 @param scene  Given scene to cast that ray into
 @return intersection struct with the appropriate values set
 */
static struct hitRecord getClosestIsect(const struct lightRay *incidentRay, const struct world *scene) {
	struct hitRecord isect;
	isect.distance = 20000.0f;
	isect.incident = *incidentRay;
	
	for (int i = 0; i < scene->sphereInstanceCount; ++i) {
		struct lightRay copy;
		copy.start = incidentRay->start;
		copy.direction = incidentRay->direction;
		copy.rayType = incidentRay->rayType;
		
		transformVector(&copy.start, scene->sphereInstances[i].composite.Ainv);
		transformDirection(&copy.direction, scene->sphereInstances[i].composite.Ainv);
		
		if (rayIntersectsWithSphere(incidentRay, scene->sphereInstances[i].object, &isect)) {
			isect.end = ((struct sphere*)scene->sphereInstances[i].object)->material;
			isect.instIndex = i;
		}
	 }
	
#ifdef LINEAR
	for (int i = 0; i < scene->instanceCount; ++i) {
		struct lightRay copy;
		copy.start = incidentRay->start;
		copy.direction = incidentRay->direction;
		copy.rayType = incidentRay->rayType;
		
		transformVector(&copy.start, scene->instances[i].composite.Ainv);
		transformDirection(&copy.direction, scene->instances[i].composite.Ainv);
		
		if (traverseBottomLevelBvh(scene->instances[i].object, &copy, &isect)) {
			isect.end = ((struct mesh*)scene->instances[i].object)->materials[isect.polygon->materialIndex];
			isect.instIndex = i;
			computeSurfaceProps(isect.polygon, &isect.uv, &isect.hitPoint, &isect.surfaceNormal);
			if (isect.end.hasNormalMap)
				isect.surfaceNormal = bumpmap(&isect);
		}
	}
#else
	if (traverseTopLevelBvh(scene->meshes, scene->topLevel, incidentRay, &isect)) {
		isect.end = scene->meshes[isect.meshIndex].materials[isect.polygon->materialIndex];
		computeSurfaceProps(isect.polygon, &isect.uv, &isect.hitPoint, &isect.surfaceNormal);
		if (isect.end.hasNormalMap)
			isect.surfaceNormal = bumpmap(&isect);
	}
#endif
	return isect;
}

static struct color getHDRI(const struct lightRay *incidentRay, const struct hdr *hdr) {
	//Unit direction vector
	struct vector ud = vecNormalize(incidentRay->direction);
	
	//To polar from cartesian
	float r = 1.0f; //Normalized above
	float phi = (atan2f(ud.z, ud.x)/4) + hdr->offset;
	float theta = acosf((-ud.y/r));
	
	float u = theta / PI;
	float v = (phi / (PI/2));
	
	u = wrapMinMax(u, 0, 1);
	v = wrapMinMax(v, 0, 1);
	
	float x = (v * hdr->t->width);
	float y = (u * hdr->t->height);
	
	struct color newColor = textureGetPixelFiltered(hdr->t, x, y);
	
	return newColor;
}

//Linearly interpolate based on the Y component
static struct color getAmbientColor(const struct lightRay *incidentRay, struct gradient color) {
	struct vector unitDirection = vecNormalize(incidentRay->direction);
	float t = 0.5f * (unitDirection.y + 1.0f);
	return addColors(colorCoef(1.0f - t, color.down), colorCoef(t, color.up));
}

static struct color getBackground(const struct lightRay *incidentRay, const struct world *scene) {
	return scene->hdr ? getHDRI(incidentRay, scene->hdr) : getAmbientColor(incidentRay, scene->ambientColor);
}
