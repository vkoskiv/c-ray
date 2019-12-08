//
//  sphere.h
//  C-ray
//
//  Created by Valtteri Koskivuori on 28/02/2015.
//  Copyright © 2015-2019 Valtteri Koskivuori. All rights reserved.
//

#pragma once

//Sphere
struct sphere {
	struct vector pos;
	float radius;
	struct material material;
};

//New sphere with given params
struct sphere newSphere(struct vector pos, float radius, struct material material);

struct sphere newLightSphere(struct vector pos, float radius, struct color color, float intensity);

struct sphere defaultSphere(void);

//Calculates intersection between a light ray and a sphere
bool rayIntersectsWithSphere(struct sphere *sphere, struct lightRay *ray, struct hitRecord *isect);
