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
	double radius;
	struct material material;
};

struct lightRay;

//New sphere with given params
struct sphere newSphere(struct vector pos, double radius, struct material material);

struct sphere newLightSphere(struct vector pos, double radius, struct color color, double intensity);

//Calculates intersection between a light ray and a sphere
bool rayIntersectsWithSphere(struct sphere *sphere, struct lightRay *ray, struct intersection *isect);
