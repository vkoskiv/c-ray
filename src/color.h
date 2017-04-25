//
//  color.h
//  C-Ray
//
//  Created by Valtteri Koskivuori on 28/02/15.
//  Copyright (c) 2015 Valtteri Koskivuori. All rights reserved.
//
#pragma once

//Color
struct color {
	double red, green, blue, alpha;
};

//material
struct material {
	char name[MATERIAL_NAME_SIZE];
	char textureFilename[OBJ_FILENAME_LENGTH];
	struct color ambient;
	struct color diffuse;
	struct color specular;
	double reflectivity;
	double refractivity;
	double transparency;
	double sharpness;
	double glossiness;
	double refractionIndex;
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

//temporary newMaterial func
struct material newMaterial(struct color diffuse, double reflectivity);
