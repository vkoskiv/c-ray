//
//  color.h
//  c-ray
//
//  Created by Valtteri Koskivuori on 28/02/2015.
//  Copyright © 2015-2022 Valtteri Koskivuori. All rights reserved.
//
#pragma once

#include <math.h>
#include <stdbool.h>
#include <stddef.h>

//Color
struct color {
	float red, green, blue, alpha;
};

struct hsl {
	float h, s, l;
};

struct hsv {
	float h, s, v;
};

//Some standard colours
extern const struct color g_red_color;
extern const struct color g_green_color;
extern const struct color g_blue_color;
extern const struct color g_black_color;
extern const struct color g_gray_color;
extern const struct color g_white_color;
extern const struct color g_clear_color;
extern const struct color g_pink_color;

//Colors for the SDL elements
extern const struct color g_background_color;
extern const struct color g_frame_color;
extern const struct color g_prog_color;

//Multiply two colors
static inline struct color colorMul(struct color c1, struct color c2) {
	return (struct color){c1.red * c2.red, c1.green * c2.green, c1.blue * c2.blue, c1.alpha * c2.alpha};
}

//Add two colors
static inline struct color colorAdd(struct color c1, struct color c2) {
	return (struct color){c1.red + c2.red, c1.green + c2.green, c1.blue + c2.blue, c1.alpha + c2.alpha};
}

// Formula from http://alienryderflex.com/hsp.html
static inline struct color colorToGrayscale(struct color c) {
	float b = sqrtf(0.299f * powf(c.red, 2) + 0.587f * powf(c.green, 2) + 0.114f * powf(c.blue, 2));
	return (struct color){b, b, b, c.alpha};
}

//Multiply a color with a coefficient value
static inline struct color colorCoef(float coef, struct color c) {
	return (struct color){c.red * coef, c.green * coef, c.blue * coef, c.alpha * coef};
}

//Linear interpolation mix
static inline struct color colorMix(struct color c1, struct color c2, float coeff) {
	return colorAdd(colorCoef(1.0f - coeff, c1), colorCoef(coeff, c2));
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
static inline struct color colorToSRGB(struct color c) {
	return (struct color) {
		.red = linearToSRGB(c.red),
		.green = linearToSRGB(c.green),
		.blue = linearToSRGB(c.blue),
		.alpha = c.alpha
	};
}

//Convert from sRGB to linear (0.0-1.0)
static inline struct color colorFromSRGB(struct color c) {
	return (struct color) {
		.red = SRGBToLinear(c.red),
		.green = SRGBToLinear(c.green),
		.blue = SRGBToLinear(c.blue),
		.alpha = c.alpha
	};
}

static inline struct color colorLerp(struct color start, struct color end, float t) {
	return colorMix(start, end, t);
}

static inline bool colorEquals(struct color a, struct color b) {
	return a.red == b.red && a.green == b.green && a.blue == b.blue && a.alpha == b.alpha;
}

static inline void nan_clamp(struct color *a, struct color *b) {
	if (a->red != a->red || a->green != a->green || a->blue != a->blue || a->alpha != a->alpha)
		*a = *b;
}

struct color colorForKelvin(float kelvin);

// NOTE:
// h => [0,360]
// s => [0,100]
// l => [0,100]
struct color hsl_to_rgb(struct hsl in);
struct hsl rgb_to_hsl(struct color in);
struct color hsv_to_rgb(struct hsv in);
struct hsv rgb_to_hsv(struct color in);

void color_dump(struct color c, char *buf, size_t bufsize);
