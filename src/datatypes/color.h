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

typedef struct color {
	float red, green, blue, alpha;
} color;

//Color
struct gradient {
	color down;
	color up;
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
extern struct color backgroundColor;

//Return a color with given values
color colorWithValues(float red, float green, float blue, float alpha);

//Multiply two colors and return the resulting color
color multiplyColors(color c1, color c2);

//Add two colors and return the resulting color
color addColors(color c1, color c2);
color subtractColors(color c1, color c2);
color colorCoef(color c, float coeff);
color colorDivCoef(color c, float coef);
color colorAddCoef(color c, float coef);

color grayscale(color c);
float colorLength(color c);

//Multiply a color by a coefficient and return the resulting color
color multiply(color c1, color c2);
color multiplyCoef(color c, float coeff);
color colorCoefRGB(color c, float coef);

color lerp(color c1, color c2, float coeff);

color mixColors(color c1, color c2, float coeff);

color toSRGB(color c);
color fromSRGB(color c);
