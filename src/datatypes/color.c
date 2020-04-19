//
//  color.c
//  C-ray
//
//  Created by Valtteri Koskivuori on 28/02/2015.
//  Copyright © 2015-2020 Valtteri Koskivuori. All rights reserved.
//

#include "../includes.h"
#include "color.h"

//Some standard colours
//TODO: Prefix these so it's obvious they are extern variables
struct color redColor =   {1.0f, 0.0f, 0.0f, 1.0f};
struct color greenColor = {0.0f, 1.0f, 0.0f, 1.0f};
struct color blueColor =  {0.0f, 0.0f, 1.0f, 1.0f};
struct color blackColor = {0.0f, 0.0f, 0.0f, 1.0f};
struct color grayColor =  {0.5f, 0.5f, 0.5f, 1.0f};
struct color whiteColor = {1.0f, 1.0f, 1.0f, 1.0f};
struct color clearColor = {0.0f, 0.0f, 0.0f, 0.0f};

//Colors for the SDL elements
struct color backgroundColor = {0.1921568627f, 0.2f, 0.2117647059f, 1.0f};
struct color frameColor = {1.0f, 0.5f, 0.0f, 1.0f};
struct color progColor  = {0.2549019608f, 0.4509803922f, 0.9607843137f, 1.0f};

//Color functions
//Return a color with given values
struct color colorWithValues(float red, float green, float blue, float alpha) {
	return (struct color){red, green, blue, alpha};
}

struct color colorWithRGBAValues(unsigned char R, unsigned char G, unsigned char B, unsigned char A) {
	return (struct color){R / 255.0f, G / 255.0f, B / 255.0f, A / 255.0f};
}

//Multiply two colors
struct color multiplyColors(struct color c1, struct color c2) {
	return (struct color){c1.red * c2.red, c1.green * c2.green, c1.blue * c2.blue, c1.alpha * c2.alpha};
}

//Add two colors
struct color addColors(struct color c1, struct color c2) {
	return (struct color){c1.red + c2.red, c1.green + c2.green, c1.blue + c2.blue, c1.alpha + c2.alpha};
}

struct color grayscale(struct color c) {
	float b = (c.red+c.green+c.blue) / 3.0f;
	return (struct color){b, b, b, 0.0f};
}

//Multiply a color with a coefficient value
struct color colorCoef(float coef, struct color c) {
	return (struct color){c.red * coef, c.green * coef, c.blue * coef, c.alpha * coef};
}

struct color add(struct color c1, struct color c2) {
	struct color result = (struct color){0.0f, 0.0f, 0.0f, 0.0f};
	result.red = c1.red + c2.red;
	result.green = c1.green + c2.green;
	result.blue = c1.blue + c2.blue;
	result.alpha = c1.alpha + c2.alpha;
	return result;
}

struct color muls(struct color c, float coeff) {
	struct color result = (struct color){0.0f, 0.0f, 0.0f, 0.0f};
	result.red = c.red * coeff;
	result.green = c.green * coeff;
	result.blue = c.blue * coeff;
	result.alpha = c.alpha * coeff;
	return result;
}

struct color mixColors(struct color c1, struct color c2, float coeff) {
	//Linear interpolation mix
	return add(muls(c1, 1.0f - coeff), muls(c2, coeff));
}

//TODO: Move to own file
//sRGB transforms are from https://en.wikipedia.org/wiki/SRGB

float linearToSRGB(float channel) {
	if (channel <= 0.0031308f) {
		return 12.92f * channel;
	} else {
		return (1.055f * powf(channel, 0.4166666667f)) - 0.055f;
	}
}

float SRGBToLinear(float channel) {
	if (channel <= 0.04045f) {
		return channel / 12.92f;
	} else {
		return powf(((channel + 0.055f) / 1.055f), 2.4f);
	}
}

//Convert from linear (0.0-1.0) to sRGB
struct color toSRGB(struct color c) {
	struct color srgb = (struct color){0.0f, 0.0f, 0.0f, 0.0f};
	srgb.red = linearToSRGB(c.red);
	srgb.green = linearToSRGB(c.green);
	srgb.blue = linearToSRGB(c.blue);
	srgb.alpha = c.alpha;
	return srgb;
}

//Convert from sRGB to linear (0.0-1.0)
struct color fromSRGB(struct color c) {
	struct color linear = (struct color){0.0f, 0.0f, 0.0f, 0.0f};
	linear.red = SRGBToLinear(c.red);
	linear.green = SRGBToLinear(c.green);
	linear.blue = SRGBToLinear(c.blue);
	linear.alpha = c.alpha;
	return linear;
}

struct color lerp(struct color start, struct color end, float t) {
	struct color ret = {0};
	ret.red = start.red + (end.red - start.red) * t;
	ret.green = start.green + (end.green - start.green) * t;
	ret.blue = start.blue + (end.blue - start.blue) * t;
	ret.alpha = start.alpha + (end.alpha - start.alpha) * t;
	return ret;
}

// This algorithm is from Tanner Helland:
// http://www.tannerhelland.com/4435/convert-temperature-rgb-algorithm-code/
struct color colorForKelvin(float kelvin) {
	struct color ret = {0};
	float temp = kelvin >= 40000.0f ? 40000.0f : kelvin;
	temp = temp / 100.0f;
	//Red
	if (temp <= 66.0f) {
		ret.red = 255.0f;
	} else {
		ret.red = temp - 60.0f;
		ret.red = 329.698727446f * powf(ret.red, -0.1332047592f);
		ret.red = ret.red < 0.0f ? 0.0f : ret.red;
		ret.red = ret.red > 255.0f ? 255.0f : ret.red;
	}
	
	//Green
	if (temp <= 66.0f) {
		ret.green = temp;
		ret.green = 99.4708025861f * logf(ret.green) - 161.1195681661f;
		ret.green = ret.green < 0.0f ? 0.0f : ret.green;
		ret.green = ret.green > 255.0f ? 255.0f : ret.green;
	} else {
		ret.green = temp - 60.0f;
		ret.green = 288.1221695283f * powf(ret.green, -0.0755148492f);
		ret.green = ret.green < 0.0f ? 0.0f : ret.green;
		ret.green = ret.green > 255.0f ? 255.0f : ret.green;
	}
	
	//Blue
	if (temp >= 66.0f) {
		ret.blue = 255.0f;
	} else {
		if (temp <= 19.0f) {
			ret.blue = 0.0f;
		} else {
			ret.blue = temp - 10.0f;
			ret.blue = 138.5177312231f * logf(ret.blue) - 305.0447927307f;
			ret.blue = ret.blue < 0.0f ? 0.0f : ret.blue;
			ret.blue = ret.blue > 255.0f ? 255.0f : ret.blue;
		}
	}
	
	return colorWithValues(ret.red / 255.0f, ret.green / 255.0f, ret.blue / 255.0f, 0);
}
