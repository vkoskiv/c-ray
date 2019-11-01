//
//  color.c
//  C-ray
//
//  Created by Valtteri Koskivuori on 28/02/2015.
//  Copyright © 2015-2019 Valtteri Koskivuori. All rights reserved.
//

#include "../includes.h"
#include "color.h"

//Some standard colours
//TODO: Prefix these so it's obvious they are extern variables
color redColor =   {1.0, 0.0, 0.0};
color greenColor = {0.0, 1.0, 0.0};
color blueColor =  {0.0, 0.0, 1.0};
color blackColor = {0.0, 0.0, 0.0};
color grayColor =  {0.5, 0.5, 0.5};
color whiteColor = {1.0, 1.0, 1.0};
color frameColor = {1.0, 0.5, 0.0};
color clearColor = {0.0, 0.0, 0.0};
color progColor  = {0.2549019608, 0.4509803922, 0.9607843137};

//Color functions
//Return a color with given values
color colorWithValues(float red, float green, float blue, float alpha) {
	return (color){red, green, blue};
}

color colorWithRGBValues(int R, int G, int B) {
	return (color){R / 255.0, G / 255.0, B / 255.0};
}

//Multiply two colors
color multiplyColors(color c1, color c2) {
	return vec3_mul(c1, c2);
}

//Add two colors
color addColors(color c1, color c2) {
	return (color){c1.r + c2.r, c1.g + c2.g, c1.b + c2.b};
}

color grayscale(color c) {
	float b = (c.r+c.g+c.b)/3;
	return (color){b, b, b};
}

//Multiply a color with a coefficient value
color colorCoef(float coef, color c) {
	return (color){c.r * coef, c.g * coef, c.b * coef};
}

color add(color c1, color c2) {
	color result = (color){0, 0, 0};
	result.r = c1.r + c2.r;
	result.g = c1.g + c2.g;
	result.b = c1.b + c2.b;
	//result.a = c1.a + c2.a;
	return result;
}

color muls(color c, float coeff) {
	color result = (color){0, 0, 0};
	result.r = c.r * coeff;
	result.g = c.g * coeff;
	result.b = c.b * coeff;
	//result.alpha = c.alpha * coeff;
	return result;
}

color mixColors(color c1, color c2, float coeff) {
	//Linear interpolation mix
	return add(muls(c1, 1.0f - coeff), muls(c2, coeff));
}

//sRGB transforms are from https://en.wikipedia.org/wiki/SRGB

float linearToSRGB(float channel) {
	if (channel <= 0.0031308) {
		return 12.92 * channel;
	} else {
		return (1.055 * pow(channel, 0.4166666667)) - 0.055;
	}
}

float SRGBToLinear(float channel) {
	if (channel <= 0.04045) {
		return channel / 12.92;
	} else {
		return pow(((channel + 0.055) / 1.055), 2.4);
	}
}

//Convert from linear (0.0-1.0) to sRGB
color toSRGB(color c) {
	color srgb = (color){ 0.0, 0.0, 0.0 };
	srgb.r = linearToSRGB(c.r);
	srgb.g = linearToSRGB(c.g);
	srgb.b = linearToSRGB(c.b);
	//srgb.a = c.a;
	return srgb;
}

//Convert from sRGB to linear (0.0-1.0)
color fromSRGB(color c) {
	color linear = (color){0.0, 0.0, 0.0};
	linear.r = SRGBToLinear(c.r);
	linear.g = SRGBToLinear(c.g);
	linear.b = SRGBToLinear(c.b);
	//linear.a = c.a;
	return linear;
}

color lerp(color start, color end, float t) {
	color ret = {0};
	ret.r = start.r + (end.r - start.r) * t;
	ret.g = start.g + (end.g - start.g) * t;
	ret.b = start.b + (end.b - start.b) * t;
	//ret.alpha = start.alpha + (end.alpha - start.alpha) * t;
	return ret;
}
