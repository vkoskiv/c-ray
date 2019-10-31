//
//  color.h
//  C-ray
//
//  Created by Valtteri Koskivuori on 28/02/2015.
//  Copyright © 2015-2019 Valtteri Koskivuori. All rights reserved.
//
#pragma once

#define AIR_IOR 1.0

#define NOT_REFRACTIVE 1
#define NOT_REFLECTIVE -1

#include "vec3.h"

//Color
struct color {
	float red, green, blue, alpha;
};

struct gradient {
	vec3 down;
	vec3 up;
};

//Some standard colours
extern struct color redColor;
extern struct color greenColor;
extern struct color blueColor;
extern struct color blackColor;
extern struct color grayColor;
extern struct color whiteColor;
extern struct color frameColor;
extern struct color clearColor;
extern struct color progColor;

//Return a color with given values
struct color colorWithValues(float red, float green, float blue, float alpha);

//Multiply two colors and return the resulting color
struct color multiplyColors(struct color c1, struct color c2);

//Add two colors and return the resulting color
struct color addColors(struct color c1, struct color c2);

struct color grayscale(struct color c);

//Multiply a color by a coefficient and return the resulting color
struct color colorCoef(float coef, struct color c);

struct color mixColors(struct color c1, struct color c2, float coeff);

vec3 toSRGB(vec3 c);

<<<<<<< HEAD
vec3 fromSRGB(vec3 c);
=======
struct color fromSRGB(struct color c);

struct color lerp(struct color start, struct color end, float t);
>>>>>>> 1d60640fe22419135cd05015879227d4992e474f
