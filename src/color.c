//
//  color.c
//  C-Ray
//
//  Created by Valtteri Koskivuori on 28/02/15.
//  Copyright (c) 2015 Valtteri Koskivuori. All rights reserved.
//

#include "color.h"

//Some standard colours
color redColor =   {1.0, 0.0, 0.0};
color greenColor = {0.0, 1.0, 0.0};
color blueColor =  {0.0, 0.0, 1.0};
color blackColor = {0.0, 0.0, 0.0};
color whiteColor = {1.0, 1.0, 1.0};
color frameColor = {1.0, 0.5, 0.0};

//Color functions
//Return a color with given values
color colorWithValues(double red, double green, double blue, double alpha) {
	color result;
	result.red = red;
	result.green = green;
	result.blue = blue;
	result.alpha = alpha;
	return result;
}

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

//FIXME: Temporary, eventually support full OBJ spec
material newMaterial(color diffuse, double reflectivity) {
    material newMaterial;
    newMaterial.reflectivity = reflectivity;
    newMaterial.diffuse = diffuse;
    return newMaterial;
}
