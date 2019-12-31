//
//  color.h
//  C-ray
//
//  Created by Valtteri Koskivuori on 28/02/2015.
//  Copyright © 2015-2020 Valtteri Koskivuori. All rights reserved.
//
#pragma once

#define AIR_IOR 1.0

#define NOT_REFRACTIVE 1
#define NOT_REFLECTIVE -1

//TODO: Split this into color, colorspace and gradient

//Color
struct color {
	float red, green, blue, alpha;
};

struct gradient {
	struct color down;
	struct color up;
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
struct color colorWithValues(float red, float green, float blue, float alpha);

//Multiply two colors and return the resulting color
struct color multiplyColors(struct color c1, struct color c2);

//Add two colors and return the resulting color
struct color addColors(struct color c1, struct color c2);

struct color grayscale(struct color c);

//Multiply a color by a coefficient and return the resulting color
struct color colorCoef(float coef, struct color c);

struct color mixColors(struct color c1, struct color c2, float coeff);

struct color toSRGB(struct color c);

struct color fromSRGB(struct color c);

struct color lerp(struct color start, struct color end, float t);
