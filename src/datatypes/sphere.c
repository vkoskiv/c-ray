//
//  sphere.c
//  C-ray
//
//  Created by Valtteri Koskivuori on 28/02/2015.
//  Copyright © 2015-2020 Valtteri Koskivuori. All rights reserved.
//

#include "../includes.h"
#include "sphere.h"

#include "../renderer/pathtrace.h"
#include "lightray.h"

struct sphere defaultSphere() {
	return (struct sphere){ 10.0f, NULL, g_black_color, 0.0f };
}

//Calculates intersection with a sphere and a light ray
bool intersect(const struct lightRay *ray, const struct sphere *sphere, float *t) {
	//Vector dot product of the direction
	float A = vecDot(ray->direction, ray->direction);
	
	//Distance between start of a lightRay and the sphere position
	float B = 2.0f * vecDot(ray->direction, ray->start);
	
	float C = vecDot(ray->start, ray->start) - (sphere->radius * sphere->radius);
	
	float trigDiscriminant = B * B - 4.0f * A * C;

	//If discriminant is negative, no real roots and the ray has missed the sphere
	if (trigDiscriminant < 0.0f)
		return false;

	float sqrtOfDiscriminant = sqrtf(trigDiscriminant);
	float t0 = (-B + sqrtOfDiscriminant) / 2.0f;
	float t1 = (-B - sqrtOfDiscriminant) / 2.0f;

	//Pick closest intersection
	if (t0 > t1 && t1 > 0.0f) {
		t0 = t1;
	}

	//Verify intersection is larger than 0 and less than the original distance
	if (t0 < 0.00001f || t0 > *t)
		return false;

	*t = t0;
	return true;
}

bool rayIntersectsWithSphere(const struct lightRay *ray, const struct sphere *sphere, struct hitRecord *isect) {
	if (intersect(ray, sphere, &isect->distance)) {
		//Compute normal and store it to isect
		isect->hitPoint = alongRay(ray, isect->distance);
		isect->surfaceNormal = vecNormalize(isect->hitPoint);
		isect->polygon = NULL;
		return true;
	}
	return false;
}
