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
typedef vec3 color;

//Color
struct gradient {
	vec3 down;
	vec3 up;
};

//Some standard colours
extern color redColor;
extern color greenColor;
extern color blueColor;
extern color blackColor;
extern color grayColor;
extern color whiteColor;
extern color frameColor;
extern color clearColor;
extern color progColor;

//Return a color with given values
color colorWithValues(float red, float green, float blue, float alpha);

//Multiply two colors and return the resulting color
color multiplyColors(color c1, color c2);

//Add two colors and return the resulting color
color addColors(color c1, color c2);

color grayscale(color c);

//Multiply a color by a coefficient and return the resulting color
color colorCoef(float coef, color c);

color mixColors(color c1, color c2, float coeff);

color toSRGB(color c);
color fromSRGB(color c);
