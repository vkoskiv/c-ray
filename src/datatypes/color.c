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

color redColor =   {1.0, 0.0, 0.0, 1.0};
color greenColor = {0.0, 1.0, 0.0, 1.0};
color blueColor =  {0.0, 0.0, 1.0, 1.0};
color blackColor = {0.0, 0.0, 0.0, 1.0};
color grayColor =  {0.5, 0.5, 0.5, 1.0};
color whiteColor = {1.0, 1.0, 1.0, 1.0};
color clearColor = {0.0, 0.0, 0.0, 0.0};

//Colors for the SDL elements
color backgroundColor = {0.2f, 0.2f, 0.2f, 1.0f};//{0.1921568627, 0.2, 0.2117647059, 1.0};
color frameColor = {1.0, 0.5, 0.0, 1.0};
color progColor  = {0.2549019608, 0.4509803922, 0.9607843137, 1.0};

//Color functions
//Return a color with given values
color colorWithValues(float red, float green, float blue, float alpha) {
	return (color){red, green, blue, alpha};
}

//Multiply two colors
color multiplyColors(color c1, color c2) {
	return (struct color){c1.red * c2.red, c1.green * c2.green, c1.blue * c2.blue, 1.0f};
}

//Add two colors
color addColors(color c1, color c2) {
	return (struct color){c1.red + c2.red, c1.green + c2.green, c1.blue + c2.blue, 1.0f};
}

color subtractColors(color c1, color c2) {
	return (struct color){c1.red - c2.red, c1.green - c2.green, c1.blue - c2.blue, 1.0f};
}

color grayscale(color c) {
	float b = (c.red+c.green+c.blue)/3;
	return (struct color){b, b, b, 1.0};
}

float colorLength(color c) {
	float len = sqrtf(c.red * c.red + c.blue * c.blue + c.green * c.green);
	return len;
}

color colorCoefRGB(color c, float coef) {
	return (struct color) { c.red* coef, c.green* coef, c.blue* coef, 1.0f };
}

//Multiply a color with a coefficient value
color colorCoef(color c, float coef) {
	return (struct color){c.red * coef, c.green * coef, c.blue * coef, c.alpha * coef};
}

color colorDivCoef(color c, float coef) {
	return (struct color){c.red / coef, c.green / coef, c.blue / coef, c.alpha / coef};
}

color colorAddCoef(color c, float coef) {
	return (struct color){c.red + coef, c.green + coef, c.blue + coef, c.alpha + coef};
}

color multiply(color c1, color c2) {
	color result = (color){ 0.0f, 0.0f, 0.0f, 0.0f };
	result.red = c1.red * c2.red;
	result.green = c1.green * c2.green;
	result.blue = c1.blue * c2.blue;
	result.alpha = c1.alpha * c2.alpha;
	return result;
}

color mixColors(color c1, color c2, float coeff) {
	//Linear interpolation mix
	return addColors(colorCoef(c1, 1.0f - coeff), colorCoef(c2, coeff));
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
	color srgb = (color){ 0.0, 0.0, 0.0, 0.0 };
	srgb.red = linearToSRGB(c.red);
	srgb.green = linearToSRGB(c.green);
	srgb.blue = linearToSRGB(c.blue);
	srgb.alpha = c.alpha;
	return srgb;
}

//Convert from sRGB to linear (0.0-1.0)
color fromSRGB(color c) {
	color linear = (color){0.0, 0.0, 0.0, 0.0};
	linear.red = SRGBToLinear(c.red);
	linear.green = SRGBToLinear(c.green);
	linear.blue = SRGBToLinear(c.blue);
	linear.alpha = c.alpha;
	return linear;
}

color lerp(color start, color end, float t) {
	color ret = {0};
	ret.red = start.red + (end.red - start.red) * t;
	ret.green = start.green + (end.green - start.green) * t;
	ret.blue = start.blue + (end.blue - start.blue) * t;
	ret.alpha = start.alpha + (end.alpha - start.alpha) * t;
	return ret;
}
