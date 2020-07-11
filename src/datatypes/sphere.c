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

struct sphere defaultSphere() {
	return (struct sphere){vecZero(), 10.0f, defaultMaterial()};
}

//Calculates intersection with a sphere and a light ray
static inline bool intersect(const struct lightRay *ray, const struct sphere *sphere, float *t) {
	//Vector dot product of the direction
	float A = vecDot(ray->direction, ray->direction);
	
	//Distance between start of a lightRay and the sphere position
	struct vector distance = vecSub(ray->start, sphere->pos);
	
	float B = 2.0f * vecDot(ray->direction, distance);
	
	float C = vecDot(distance, distance) - (sphere->radius * sphere->radius);
	
	float trigDiscriminant = B * B - 4.0f * A * C;
	
	if (trigDiscriminant < 0.0f) return false;
	
	//If discriminant is negative, no real roots and the ray has missed the sphere
	float sqrtOfDiscriminant = sqrtf(trigDiscriminant);
	float t0 = (-B + sqrtOfDiscriminant)/(2.0f);
	float t1 = (-B - sqrtOfDiscriminant)/(2.0f);
	
	//Pick closest intersection
	if (t0 > t1) {
		t0 = t1;
	}
	
	//Verify intersection is larger than 0 and less than the original distance
	if ((t0 > 0.00001f) && (t0 < *t)) {
		*t = t0;
		return true;
	} else {
		return false;
	}
}

bool rayIntersectsWithSphere(const struct lightRay *ray, const struct sphere *sphere, struct hitRecord *isect) {
	//Pass the distance value to rayIntersectsWithSphere, where it's set
	if (intersect(ray, sphere, &isect->distance)) {
		isect->type = hitTypeSphere;
		//Compute normal and store it to isect
		struct vector hitpoint = alongRay(*ray, isect->distance);
		struct vector bitangent = vecSub(hitpoint, sphere->pos);
		isect->surfaceNormal = vecScale(bitangent, invsqrt(vecDot(bitangent, bitangent)));
		//Also store hitpoint
		isect->hitPoint = hitpoint;
		return true;
	} else {
		isect->type = hitTypeNone;
		return false;
	}
}
