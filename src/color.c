//
//  color.c
//  C-Ray
//
//  Created by Valtteri Koskivuori on 28/02/15.
//  Copyright (c) 2015 Valtteri Koskivuori. All rights reserved.
//

#include "color.h"

//Color functions
//Multiply two colors
color multiplyColors(color *c1, color *c2) {
	color result = {c1->red * c2->red, c1->green * c2->green, c1->blue * c2->blue};
	return result;
}

//Add two colors
color addColors(color *c1, color *c2) {
	color result = {c1->red + c2->red, c1->green + c2->green, c1->blue + c2->blue};
	return result;
}

//Multiply a color with a coefficient value
color colorCoef(double coef, color *c) {
	color result = {c->red * coef, c->green * coef, c->blue * coef};
	return result;
}