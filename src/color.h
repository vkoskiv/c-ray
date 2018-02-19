//
//  color.h
//  C-Ray
//
//  Created by Valtteri Koskivuori on 28/02/15.
//  Copyright (c) 2015 Valtteri Koskivuori. All rights reserved.
//
#pragma once

#define AIR_IOR 1.0

#define NOT_REFRACTIVE 1
#define NOT_REFLECTIVE -1

/*
 From: https://blenderartists.org/forum/showthread.php?71202-Material-IOR-Value-reference
 'Air': 1.000
 'Bubble': 1.100
 'Liquid methane': 1.150
 'Ice(H2O)': 1.310
 'Water': 1.333
 'Clear Plastic': 1.400
 'Glass': 1.440 - 1.900
 'Light glass': 1.450
 'Standart glass': 1.520
 'Heavy glass': 1.650
 'Obsidian': 1.480 - 1.510
 'Onyx': 1.486 - 1.658
 'Acrylic glass': 1.491
 'Benzene': 1.501
 'Crown glass': 1.510
 'Jasper': 1.540
 'Agate': 1.544 - 1.553
 'Amethist': 1.544 - 1.553
 'Salt': 1.544
 'Amber': 1.550
 'Quartz': 1.550
 'Sugar': 1.560
 'Emerald': 1.576 - 1.582
 'Flint glass': 1.613
 'Topaz': 1.620 - 1.627
 'Jade': 1.660 - 1.680
 'Saphire': 1.760
 'Ruby': 1.760 - 2.419
 'Cristal': 1.870
 'Diamond': 2.417 - 2.541
 */

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
