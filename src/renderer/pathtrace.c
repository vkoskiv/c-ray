//
//  pathtrace.c
//  C-ray
//
//  Created by Valtteri Koskivuori on 27/04/2017.
//  Copyright © 2015-2019 Valtteri Koskivuori. All rights reserved.
//

#include "../includes.h"
#include "pathtrace.h"

#include "../datatypes/scene.h"
#include "../datatypes/camera.h"
#include "../acceleration/bbox.h"
#include "../acceleration/kdtree.h"
#include "../datatypes/texture.h"

struct intersection getClosestIsect(struct lightRay *incidentRay, struct world *scene);
struct color getBackground(struct lightRay *incidentRay, struct world *scene);

struct color pathTrace(struct lightRay *incidentRay, struct world *scene, int depth, int maxDepth, pcg32_random_t *rng) {
	struct intersection isect = getClosestIsect(incidentRay, scene);
	if (isect.didIntersect) {
		struct lightRay scattered;
		struct color attenuation;
		struct color emitted = isect.end.emission;
		if (depth < maxDepth && isect.end.bsdf(&isect, incidentRay, &attenuation, &scattered, rng)) {
			double probability = 1;
			if (depth >= 2) {
				probability = max(attenuation.red, max(attenuation.green, attenuation.blue));
				if (rndDouble(0, 1, rng) > probability) {
					return emitted;
				}
			}
			struct color newColor = pathTrace(&scattered, scene, depth + 1, maxDepth, rng);
			struct color attenuated = multiplyColors(&attenuation, &newColor);
			struct color final = addColors(&emitted, &attenuated);
			return colorCoef(1.0 / probability, &final);
		} else {
			return emitted;
		}
	} else {
		return getBackground(incidentRay, scene);
	}
}

/**
 Calculate the closest intersection point, and other relevant information based on a given lightRay and scene
 See the intersection struct for documentation of what this function calculates.

 @param incidentRay Given light ray (set up in renderThread())
 @param scene  Given scene to cast that ray into
 @return intersection struct with the appropriate values set
 */
struct intersection getClosestIsect(struct lightRay *incidentRay, struct world *scene) {
	struct intersection isect;
	isect.distance = 20000.0;
	isect.ray = *incidentRay;
	isect.start = incidentRay->currentMedium;
	isect.didIntersect = false;
	for (int i = 0; i < scene->sphereCount; i++) {
		if (rayIntersectsWithSphere(&scene->spheres[i], incidentRay, &isect)) {
			isect.end = scene->spheres[i].material;
			isect.didIntersect = true;
		}
	}
	for (int o = 0; o < scene->meshCount; o++) {
		if (rayIntersectsWithNode(scene->meshes[o].tree, incidentRay, &isect)) {
			isect.end = scene->meshes[o].materials[polygonArray[isect.polyIndex].materialIndex];
			isect.didIntersect = true;
		}
	}
	return isect;
}

float wrapMax(float x, float max) {
    return fmod(max + fmod(x, max), max);
}

float wrapMinMax(float x, float min, float max) {
    return min + wrapMax(x - min, max - min);
}

struct color getHDRI(struct lightRay *incidentRay, struct world *scene) {
	//Unit direction vector
	struct vector ud = vecNormalize(&incidentRay->direction);
	
	//To polar from cartesian
	float r = 1.0f; //Normalized above
	float phi = (atan2f(ud.z, ud.x)/4) + scene->hdr->offset;
	float theta = acosf((-ud.y/r));
	
	float u = theta / M_PI;
	float v = (phi / (M_PI/2));
	
	u = wrapMinMax(u, 0, 1);
	v = wrapMinMax(v, 0, 1);
	
	float x =  (v * *scene->hdr->width) - 0.5;
	float y = (u * *scene->hdr->height) - 0.5;
	
	struct color newColor = textureGetPixelFiltered(scene->hdr, x, y);
	
	return newColor;
}

struct color getAmbientColor(struct lightRay *incidentRay, struct gradient *color) {
	//Linearly interpolate based on the Y component, from white to light blue
	struct vector unitDirection = vecNormalize(&incidentRay->direction);
	double t = 0.5 * (unitDirection.y + 1.0);
	struct color temp1 = colorCoef(1.0 - t, color->down);
	struct color temp2 = colorCoef(t, color->up);
	return addColors(&temp1, &temp2);
}

struct color getBackground(struct lightRay *incidentRay, struct world *scene) {
	return scene->hdr ? getHDRI(incidentRay, scene) : getAmbientColor(incidentRay, scene->ambientColor);
}
