//
//  color.c
//  C-Ray
//
//  Created by Valtteri Koskivuori on 28/02/15.
//  Copyright (c) 2015 Valtteri Koskivuori. All rights reserved.
//

#include "includes.h"
#include "color.h"

//Some standard colours
struct color redColor =   {1.0, 0.0, 0.0, 0.0};
struct color greenColor = {0.0, 1.0, 0.0, 0.0};
struct color blueColor =  {0.0, 0.0, 1.0, 0.0};
struct color blackColor = {0.0, 0.0, 0.0, 0.0};
struct color whiteColor = {1.0, 1.0, 1.0, 0.0};
struct color frameColor = {1.0, 0.5, 0.0, 0.0};

//Color functions
//Return a color with given values
struct color colorWithValues(double red, double green, double blue, double alpha) {
	return (struct color){red, green, blue, alpha};
}

//Multiply two colors
struct color multiplyColors(struct color *c1, struct color *c2) {
	struct color result = {c1->red * c2->red, c1->green * c2->green, c1->blue * c2->blue, 0.0};
	return result;
}

//Add two colors
struct color addColors(struct color *c1, struct color *c2) {
	struct color result = {c1->red + c2->red, c1->green + c2->green, c1->blue + c2->blue, 0.0};
	return result;
}

//Multiply a color with a coefficient value
struct color colorCoef(double coef, struct color *c) {
	struct color result = {c->red * coef, c->green * coef, c->blue * coef, 0.0};
	return result;
}
