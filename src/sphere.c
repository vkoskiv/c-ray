//
//  sphere.c
//  C-Ray
//
//  Created by Valtteri Koskivuori on 28/02/15.
//  Copyright (c) 2015 Valtteri Koskivuori. All rights reserved.
//

#include "includes.h"
#include "sphere.h"

struct sphere newSphere(struct vector pos, double radius, int materialIndex) {
	return (struct sphere){pos, radius, materialIndex};
}

//Just check for intersection, nothing else.
bool rayIntersectsWithSphereFast(struct lightRay *ray, struct sphere *sphere) {
	double A = scalarProduct(&ray->direction, &ray->direction);
	struct vector distance = subtractVectors(&ray->start, &sphere->pos);
	double B = 2 * scalarProduct(&ray->direction, &distance);
	double C = scalarProduct(&distance, &distance) - (sphere->radius * sphere->radius);
	double trigDiscriminant = B * B - 4 * A * C;
	if (trigDiscriminant < 0) {
		return false;
	} else {
		return true;
	}
}

//Calculates intersection with a sphere and a light ray
bool rayIntersectsWithSphere(struct lightRay *ray, struct sphere *sphere, double *t) {
	bool intersects = false;
	
	//Vector dot product of the direction
	double A = scalarProduct(&ray->direction, &ray->direction);
	
	//Distance between start of a lightRay and the sphere position
	struct vector distance = subtractVectors(&ray->start, &sphere->pos);
	
	double B = 2 * scalarProduct(&ray->direction, &distance);
	
	double C = scalarProduct(&distance, &distance) - (sphere->radius * sphere->radius);
	
	double trigDiscriminant = B * B - 4 * A * C;
	
	//If discriminant is negative, no real roots and the ray has missed the sphere
	if (trigDiscriminant < 0) {
		intersects = false;
	} else {
		double sqrtOfDiscriminant = sqrt(trigDiscriminant);
		double t0 = (-B + sqrtOfDiscriminant)/(2);
		double t1 = (-B - sqrtOfDiscriminant)/(2);
		
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
