//
//  sphere.h
//  C-ray
//
//  Created by Valtteri Koskivuori on 28/02/2015.
//  Copyright © 2015-2019 Valtteri Koskivuori. All rights reserved.
//

#pragma once

#include "vec3.h"

//Sphere
struct sphere {
	vec3 pos;
	float radius;
	Material *material;
};

struct lightRay;

//New sphere with given params
struct sphere newSphere(vec3 pos, float radius, Material* material);

struct sphere newLightSphere(vec3 pos, float radius, vec3 color, float intensity);

struct sphere defaultSphere(void);

//Calculates intersection between a light ray and a sphere
bool rayIntersectsWithSphere(struct sphere *sphere, struct lightRay *ray, struct intersection *isect);
