//
//  pathtrace.c
//  C-Ray
//
//  Created by Valtteri Koskivuori on 27/04/2017.
//  Copyright © 2017 Valtteri Koskivuori. All rights reserved.
//

#include "includes.h"
#include "pathtrace.h"

#include "scene.h"
#include "camera.h"
#include "bbox.h"
#include "kdtree.h"

struct intersection getClosestIsect(struct lightRay *incidentRay, struct world *scene);
struct color getAmbientColor(struct lightRay *incidentRay);

struct color pathTrace(struct lightRay *incidentRay, struct world *scene, int depth) {
	struct intersection isect = getClosestIsect(incidentRay, scene);
	if (isect.didIntersect) {
		struct lightRay scattered;
		struct color attenuation;
		struct color emitted = isect.end.emission;
		if (depth < scene->bounces && isect.end.bsdf(&isect, incidentRay, &attenuation, &scattered)) {
			double probability = max(attenuation.red, max(attenuation.green, attenuation.blue));
			if (getRandomDouble(0, 1) > probability) {
				return emitted;
			} else {
				struct color newColor = pathTrace(&scattered, scene, depth + 1);
				struct color attenuated = multiplyColors(&attenuation, &newColor);
				struct color final = addColors(&emitted, &attenuated);
				return colorCoef(1.0 / probability, &final);
			}
		} else {
			return emitted;
		}
	} else {
		return getAmbientColor(incidentRay);
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
	memset(&isect, 0, sizeof(isect));
	
	isect.distance = 20000.0;
	isect.ray = *incidentRay;
	isect.start = incidentRay->currentMedium;
	isect.didIntersect = false;
	int objCount = scene->objCount;
	int sphereCount = scene->sphereCount;
	
	for (int i = 0; i < sphereCount; i++) {
		if (rayIntersectsWithSphere(&scene->spheres[i], incidentRay, &isect)) {
			isect.end = scene->spheres[i].material;
			isect.didIntersect = true;
		}
	}
	
	for (int o = 0; o < objCount; o++) {
		if (rayIntersectsWithNode(scene->objs[o].tree, incidentRay, &isect)) {
			isect.end = scene->objs[o].materials[polygonArray[isect.polyIndex].materialIndex];
			isect.didIntersect = true;
		}
	}
	
	isect.surfaceNormal = normalizeVector(&isect.surfaceNormal);
	
	return isect;
}

struct color getAmbientColor(struct lightRay *incidentRay) {
	//Linearly interpolate based on the Y component, from white to light blue
	struct vector unitDirection = normalizeVector(&incidentRay->direction);
	double t = 0.5 * (unitDirection.y + 1.0);
	struct color temp1 = colorCoef(1.0 - t, &(struct color){1.0, 1.0, 1.0, 0.0});
	struct color temp2 = colorCoef(t, &(struct color){0.5, 0.7, 1.0, 0.0});
	return addColors(&temp1, &temp2);
}
