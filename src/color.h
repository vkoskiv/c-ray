//
//  color.h
//  C-Ray
//
//  Created by Valtteri Koskivuori on 28/02/15.
//  Copyright (c) 2015-2018 Valtteri Koskivuori. All rights reserved.
//
#pragma once

#define AIR_IOR 1.0

#define NOT_REFRACTIVE 1
#define NOT_REFLECTIVE -1

//Color
struct color {
	double red, green, blue, alpha;
};

//Some standard colours
extern struct color redColor;
extern struct color greenColor;
extern struct color blueColor;
extern struct color blackColor;
extern struct color whiteColor;
extern struct color frameColor;

//Return a color with given values
struct color colorWithValues(double red, double green, double blue, double alpha);

//Multiply two colors and return the resulting color
struct color multiplyColors(struct color *c1, struct color *c2);

//Add two colors and return the resulting color
struct color addColors(struct color *c1, struct color *c2);

//Multiply a color by a coefficient and return the resulting color
struct color colorCoef(double coef, struct color *c);

struct color mixColors(struct color c1, struct color c2, float coeff);
