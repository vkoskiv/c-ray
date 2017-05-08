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
struct color redColor =   {1.0, 0.0, 0.0};
struct color greenColor = {0.0, 1.0, 0.0};
struct color blueColor =  {0.0, 0.0, 1.0};
struct color blackColor = {0.0, 0.0, 0.0};
struct color whiteColor = {1.0, 1.0, 1.0};
struct color frameColor = {1.0, 0.5, 0.0};

//Color functions
//Return a color with given values
struct color colorWithValues(double red, double green, double blue, double alpha) {
	return (struct color){red, green, blue, alpha};
}

//Multiply two colors
struct color multiplyColors(struct color *c1, struct color *c2) {
	struct color result = {c1->red * c2->red, c1->green * c2->green, c1->blue * c2->blue};
	return result;
}

//Add two colors
struct color addColors(struct color *c1, struct color *c2) {
	struct color result = {c1->red + c2->red, c1->green + c2->green, c1->blue + c2->blue};
	return result;
}

//Multiply a color with a coefficient value
struct color colorCoef(double coef, struct color *c) {
	struct color result = {c->red * coef, c->green * coef, c->blue * coef};
	return result;
}

//FIXME: Temporary, eventually support full OBJ spec
struct material newMaterial(struct color diffuse, double reflectivity) {
	struct material newMaterial;
	newMaterial.reflectivity = reflectivity;
	newMaterial.diffuse = diffuse;
	return newMaterial;
}

struct material newMaterialFull(struct color ambient,
							struct color diffuse,
							struct color specular,
							double reflectivity,
							double refractivity,
							double IOR, double
							transparency, double
							sharpness, double
							glossiness) {
	struct material mat;
	
	mat.ambient = ambient;
	mat.diffuse = diffuse;
	mat.specular = specular;
	mat.reflectivity = reflectivity;
	mat.refractivity = refractivity;
	mat.IOR = IOR;
	mat.transparency = transparency;
	mat.sharpness = sharpness;
	mat.glossiness = glossiness;
	
	return mat;
}
