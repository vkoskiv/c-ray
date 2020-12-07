//
//  color.h
//  C-ray
//
//  Created by Valtteri Koskivuori on 28/02/2015.
//  Copyright © 2015-2020 Valtteri Koskivuori. All rights reserved.
//
#pragma once

#include <math.h>

#define AIR_IOR 1.0

#define NOT_REFRACTIVE 1
#define NOT_REFLECTIVE -1

//TODO: Split this into color, colorspace and gradient

//Color
struct color {
	float red, green, blue, alpha;
};

struct gradient {
	struct color down;
	struct color up;
};

//Some standard colours
extern const struct color redColor;
extern const struct color greenColor;
extern const struct color blueColor;
extern const struct color blackColor;
extern const struct color grayColor;
extern const struct color whiteColor;
extern const struct color clearColor;

//Colors for the SDL elements
extern const struct color backgroundColor;
extern const struct color frameColor;
extern const struct color progColor;

static inline struct color newGrayColor(float brightness) {
	return (struct color){brightness, brightness, brightness, 1.0f};
}

static inline struct color colorWithRGBAValues(unsigned char R, unsigned char G, unsigned char B, unsigned char A) {
	return (struct color){R / 255.0f, G / 255.0f, B / 255.0f, A / 255.0f};
}

//Multiply two colors
static inline struct color multiplyColors(struct color c1, struct color c2) {
	return (struct color){c1.red * c2.red, c1.green * c2.green, c1.blue * c2.blue, c1.alpha * c2.alpha};
}

//Add two colors
static inline struct color addColors(struct color c1, struct color c2) {
	return (struct color){c1.red + c2.red, c1.green + c2.green, c1.blue + c2.blue, c1.alpha + c2.alpha};
}

static inline struct color grayscale(struct color c) {
	float b = (c.red + c.green + c.blue) / 3.0f;
	return (struct color){b, b, b, c.alpha};
}

//Multiply a color with a coefficient value
static inline struct color colorCoef(float coef, struct color c) {
	return (struct color){c.red * coef, c.green * coef, c.blue * coef, c.alpha * coef};
}

//Linear interpolation mix
static inline struct color mixColors(struct color c1, struct color c2, float coeff) {
	return addColors(colorCoef(1.0f - coeff, c1), colorCoef(coeff, c2));
}

//TODO: Move to own file
//sRGB transforms are from https://en.wikipedia.org/wiki/SRGB

static inline float linearToSRGB(float channel) {
	if (channel <= 0.0031308f) {
		return 12.92f * channel;
	} else {
		return (1.055f * powf(channel, 0.4166666667f)) - 0.055f;
	}
}

static inline float SRGBToLinear(float channel) {
	if (channel <= 0.04045f) {
		return channel / 12.92f;
	} else {
		return powf(((channel + 0.055f) / 1.055f), 2.4f);
	}
}

//Convert from linear (0.0-1.0) to sRGB
static inline struct color toSRGB(struct color c) {
	return (struct color) {
		.red = linearToSRGB(c.red),
		.green = linearToSRGB(c.green),
		.blue = linearToSRGB(c.blue),
		.alpha = c.alpha
	};
}

//Convert from sRGB to linear (0.0-1.0)
static inline struct color fromSRGB(struct color c) {
	return (struct color) {
		.red = SRGBToLinear(c.red),
		.green = SRGBToLinear(c.green),
		.blue = SRGBToLinear(c.blue),
		.alpha = c.alpha
	};
}

static inline struct color lerp(struct color start, struct color end, float t) {
	return mixColors(start, end, t);
}

static inline bool colorEquals(struct color a, struct color b) {
	return a.red == b.red && a.green == b.green && a.blue == b.blue && a.alpha == b.alpha;
}

struct color colorForKelvin(float kelvin);
