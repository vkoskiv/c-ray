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

static struct hitRecord getClosestIsect(struct lightRay *incidentRay, const struct world *scene, sampler *sampler);
static struct color getBackground(const struct lightRay *incidentRay, const struct world *scene);

struct color debugNormals(const struct lightRay *incidentRay, const struct world *scene, int maxDepth, sampler *sampler) {
	(void)maxDepth;
	(void)sampler;
	struct lightRay currentRay = *incidentRay;
	struct hitRecord isect = getClosestIsect(&currentRay, scene, sampler);
	if (isect.instIndex < 0)
		return getBackground(&currentRay, scene);
	struct vector normal =  isect.surfaceNormal;
	return colorWithValues(fabs(normal.x), fabs(normal.y), fabs(normal.z), 1.0f);
}

struct color pathTrace(const struct lightRay *incidentRay, const struct world *scene, int maxDepth, sampler *sampler) {
#ifdef DBG_NORMALS
	return debugNormals(incidentRay, scene, maxDepth, sampler);
#endif
	struct color weight = whiteColor; // Current path weight
	struct color finalColor = blackColor; // Final path contribution
	struct lightRay currentRay = *incidentRay;

	for (int depth = 0; depth < maxDepth; ++depth) {
		const struct hitRecord isect = getClosestIsect(&currentRay, scene, sampler);
		if (isect.instIndex < 0) {
			finalColor = addColors(finalColor, multiplyColors(weight, getBackground(&currentRay, scene)));
			break;
		}

		finalColor = addColors(finalColor, multiplyColors(weight, isect.material.emission));
		
		struct color attenuation;
		if (!isect.material.bsdf(&isect, &attenuation, &currentRay, sampler))
			break;
		
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
	struct vector ucomp = vecScale(g_vertices[p->vertexIndex[1]], u);
	struct vector vcomp = vecScale(g_vertices[p->vertexIndex[2]], v);
	struct vector wcomp = vecScale(g_vertices[p->vertexIndex[0]], w);
	
	*hitPoint = vecAdd(vecAdd(ucomp, vcomp), wcomp);
	
	if (p->hasNormals) {
		struct vector upcomp = vecScale(g_normals[p->normalIndex[1]], u);
		struct vector vpcomp = vecScale(g_normals[p->normalIndex[2]], v);
		struct vector wpcomp = vecScale(g_normals[p->normalIndex[0]], w);
		
		*normal = vecNormalize(vecAdd(vecAdd(upcomp, vpcomp), wpcomp));
	}
}

static struct vector bumpmap(const struct hitRecord *isect) {
	struct material mtl = isect->material;
	struct poly *p = isect->polygon;
	float width = mtl.normalMap->width;
	float heigh = mtl.normalMap->height;
	float u = isect->uv.x;
	float v = isect->uv.y;
	float w = 1.0f - u - v;
	struct coord ucomponent = coordScale(u, g_textureCoords[p->textureIndex[1]]);
	struct coord vcomponent = coordScale(v, g_textureCoords[p->textureIndex[2]]);
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
static struct hitRecord getClosestIsect(struct lightRay *incidentRay, const struct world *scene, sampler *sampler) {
	incidentRay->start = vecAdd(incidentRay->start, vecScale(incidentRay->direction, scene->rayOffset));
	struct hitRecord isect;
	isect.instIndex = -1;
	isect.distance = FLT_MAX;
	isect.incident = *incidentRay;
	isect.polygon = NULL;
	
	if (!traverseTopLevelBvh(scene->instances, scene->topLevel, incidentRay, &isect))
		return isect;
	
	if (isect.polygon) {
		isect.material = ((struct mesh*)scene->instances[isect.instIndex].object)->materials[isect.polygon->materialIndex];
		computeSurfaceProps(isect.polygon, &isect.uv, &isect.hitPoint, &isect.surfaceNormal);
		if (isect.material.hasNormalMap)
			isect.surfaceNormal = bumpmap(&isect);
	}
	transformPoint(&isect.hitPoint, &scene->instances[isect.instIndex].composite.A);
	transformVectorWithTranspose(&isect.surfaceNormal, &scene->instances[isect.instIndex].composite.Ainv);
	
	float prob = isect.material.hasTexture ? colorForUV(&isect).alpha : isect.material.diffuse.alpha;
	if (prob < 1.0f) {
		if (getDimension(sampler) > prob) {
			struct lightRay next = {isect.hitPoint, incidentRay->direction, rayTypeIncident};
			return getClosestIsect(&next, scene, sampler);
		}
	}
	
	isect.surfaceNormal = vecNormalize(isect.surfaceNormal);
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
