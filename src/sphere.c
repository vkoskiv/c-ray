//
//  sphere.c
//  C-Ray
//
//  Created by Valtteri Koskivuori on 28/02/15.
//  Copyright (c) 2015 Valtteri Koskivuori. All rights reserved.
//

#include "sphere.h"

sphere newSphere(vector pos, float radius, int materialIndex) {
    return (sphere){pos, radius, materialIndex};
}

//Just check for intersection, nothing else.
bool rayIntersectsWithSphereFast(lightRay *ray, sphere *sphere) {
	float A = scalarProduct(&ray->direction, &ray->direction);
	vector distance = subtractVectors(&ray->start, &sphere->pos);
	float B = 2 * scalarProduct(&ray->direction, &distance);
	float C = scalarProduct(&distance, &distance) - (sphere->radius * sphere->radius);
	float trigDiscriminant = B * B - 4 * A * C;
	if (trigDiscriminant < 0) {
		return false;
	} else {
		return true;
	}
}

//Calculates intersection with a sphere and a light ray
bool rayIntersectsWithSphere(lightRay *ray, sphere *sphere, double *t) {
	bool intersects = false;
	
	//Vector dot product of the direction
	float A = scalarProduct(&ray->direction, &ray->direction);
	
	//Distance between start of a lightRay and the sphere position
	vector distance = subtractVectors(&ray->start, &sphere->pos);

	float B = 2 * scalarProduct(&ray->direction, &distance);
	
	float C = scalarProduct(&distance, &distance) - (sphere->radius * sphere->radius);
	
	float trigDiscriminant = B * B - 4 * A * C;
	
	//If discriminant is negative, no real roots and the ray has missed the sphere
	if (trigDiscriminant < 0) {
		intersects = false;
	} else {
		float sqrtOfDiscriminant = sqrtf(trigDiscriminant);
		float t0 = (-B + sqrtOfDiscriminant)/(2);
		float t1 = (-B - sqrtOfDiscriminant)/(2);
		
		//Pick closest intersection
		if (t0 > t1) {
			t0 = t1;
		}
		
		//Verify intersection is larger than 0 and less than the original distance
		if ((t0 > 0.001f) && (t0 < *t)) {
			*t = t0;
			intersects = true;
		} else {
			intersects = false;
		}
	}
	return intersects;
}
