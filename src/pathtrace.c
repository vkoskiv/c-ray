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
#include "poly.h"
#include "light.h"
#include "obj.h"
#include "bbox.h"
#include "kdtree.h"

//TODO: Merge this functionality into rayIntersectsWithSphere
bool rayIntersectsWithSphereTemp(struct sphere *sphere, struct lightRay *ray, struct intersection *isect) {
	//Pass the distance value to rayIntersectsWithSphere, where it's set
	if (rayIntersectsWithSphere(ray, sphere, &isect->distance)) {
		isect->type = hitTypeSphere;
		//Compute normal and store it to isect
		struct vector scaled = vectorScale(isect->distance, &ray->direction);
		struct vector hitpoint = addVectors(&ray->start, &scaled);
		struct vector surfaceNormal = subtractVectors(&hitpoint, &sphere->pos);
		double temp = scalarProduct(&surfaceNormal,&surfaceNormal);
		if (temp == 0.0) return false; //FIXME: Check this later
		temp = invsqrt(temp);
		isect->surfaceNormal = vectorScale(temp, &surfaceNormal);
		//Also store hitpoint
		isect->hitPoint = hitpoint;
		return true;
	} else {
		isect->type = hitTypeNone;
		return false;
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
		if (rayIntersectsWithSphereTemp(&scene->spheres[i], incidentRay, &isect)) {
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
	float t = 0.5 * (unitDirection.y + 1.0);
	struct color temp1 = colorCoef(1.0 - t, &(struct color){1.0, 1.0, 1.0, 0.0});
	struct color temp2 = colorCoef(t, &(struct color){0.5, 0.7, 1.0, 0.0});
	return addColors(&temp1, &temp2);
}

struct color pathTrace(struct lightRay *incidentRay, struct world *scene, int depth) {
	struct intersection rec = getClosestIsect(incidentRay, scene);
	if (rec.didIntersect) {
		struct lightRay scattered = {};
		struct color attenuation = {};
		if (depth < scene->bounces && rec.end.bsdf(&rec, incidentRay, &attenuation, &scattered)) {
			struct color newColor = pathTrace(&scattered, scene, depth + 1);
			return multiplyColors(&attenuation, &newColor);
		} else {
			return (struct color){0.0, 0.0, 0.0, 0.0};
		}
	} else {
		return getAmbientColor(incidentRay);
	}
}
