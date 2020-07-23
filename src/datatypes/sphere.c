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
#include "lightRay.h"

struct sphere newSphere(float radius, struct material material) {
	return (struct sphere){radius, material};
}

struct sphere defaultSphere() {
	return (struct sphere){10.0f, defaultMaterial()};
}

//FIXME: dirty hack
struct sphere newLightSphere(float radius, struct color color, float intensity) {
	struct sphere newSphere;
	newSphere.radius = radius;
	newSphere.material = newMaterial(color, 0.0f);
	newSphere.material.emission = colorCoef(intensity, color);
	newSphere.material.type = emission;
	assignBSDF(&newSphere.material);
	return newSphere;
}

//Calculates intersection with a sphere and a light ray
bool intersect(const struct lightRay *ray, const struct sphere *sphere, float *t) {
	//Vector dot product of the direction
	float A = vecDot(ray->direction, ray->direction);
	
	//Distance between start of a lightRay and the sphere position
	float B = 2 * vecDot(ray->direction, ray->start);
	
	float C = vecDot(ray->start, ray->start) - (sphere->radius * sphere->radius);
	
	float trigDiscriminant = B * B - 4 * A * C;
	
	//If discriminant is negative, no real roots and the ray has missed the sphere
	if (trigDiscriminant < 0)
		return false;

	float sqrtOfDiscriminant = sqrtf(trigDiscriminant);
	float t0 = (-B + sqrtOfDiscriminant)/(2);
	float t1 = (-B - sqrtOfDiscriminant)/(2);

	//Pick closest intersection
	if (t0 > t1 && t1 > 0) {
		t0 = t1;
	}

	//Verify intersection is larger than 0 and less than the original distance
	if (t0 < 0.00001f || t0 > *t)
		return false;

	*t = t0;
	return true;
}

bool rayIntersectsWithSphere(const struct lightRay *ray, const struct sphere *sphere, struct hitRecord *isect) {
	//Pass the distance value to rayIntersectsWithSphere, where it's set
	if (intersect(ray, sphere, &isect->distance)) {
		//Compute normal and store it to isect
		struct vector scaled = vecScale(ray->direction, isect->distance);
		struct vector hitPoint = vecAdd(ray->start, scaled);
		isect->surfaceNormal = hitPoint;
		isect->hitPoint = hitPoint;
		isect->polygon = NULL;
		return true;
	}
	return false;
}
