//
//  color.c
//  C-ray
//
//  Created by Valtteri Koskivuori on 28/02/2015.
//  Copyright © 2015-2020 Valtteri Koskivuori. All rights reserved.
//

#include "../includes.h"
#include <stdio.h>
#include "color.h"
#include "assert.h"

//Some standard colours
const struct color g_red_color =   { 1.0f, 0.0f, 0.0f, 1.0f };
const struct color g_green_color = { 0.0f, 1.0f, 0.0f, 1.0f };
const struct color g_blue_color =  { 0.0f, 0.0f, 1.0f, 1.0f };
const struct color g_black_color = { 0.0f, 0.0f, 0.0f, 1.0f };
const struct color g_gray_color =  { 0.5f, 0.5f, 0.5f, 1.0f };
const struct color g_white_color = { 1.0f, 1.0f, 1.0f, 1.0f };
const struct color g_clear_color = { 0.0f, 0.0f, 0.0f, 0.0f };
const struct color g_pink_color  = { 1.0f, 0.0f, 0.5f, 1.0f };

//Colors for the SDL elements
const struct color g_background_color = { 0.1921568627f, 0.2f, 0.2117647059f, 1.0f };
const struct color g_frame_color = { 1.0f, 0.5f, 0.0f, 1.0f };
const struct color g_prog_color  = { 0.2549019608f, 0.4509803922f, 0.9607843137f, 1.0f };

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
	
	return (struct color){ret.red / 255.0f, ret.green / 255.0f, ret.blue / 255.0f, 1.0f};
}

// Adapted from code in this SO answer: https://stackoverflow.com/a/9493060

// It's not obvious to me what p, q and t are supposed to be short for, copied here verbatim.
static inline float hue_to_rgb(float p, float q, float t) {
	if (t < 0.0f) t += 1.0f;
	if (t > 1.0f) t -= 1.0f;
	if (t < 1.0f / 6.0f) return p + (q - p) * 6.0f * t;
	if (t < 1.0f / 2.0f) return q;
	if (t < 2.0f / 3.0f) return p + (q - p) * (2.0f / 3.0f - t) * 6.0f;
	return p;
}

struct color hsl_to_rgb(struct hsl in) {
	in.h = in.h / 360.0f;
	in.s = in.s/ 100.0f;
	in.l = in.l / 100.0f;
	if (in.s == 0.0f) {
		return (struct color){ in.l, in.l, in.l, 1.0f };
	} else {
		const float q = in.l < 0.5f ? in.l * (1.0f + in.s) : in.l + in.s - in.l * in.s;
		const float p = 2.0f * in.l - q;
		return (struct color){ hue_to_rgb(p, q, in.h + 1.0f / 3.0f), hue_to_rgb(p, q, in.h), hue_to_rgb(p, q, in.h- 1.0f / 3.0f), 1.0f };
	}
}
struct hsl rgb_to_hsl(struct color in) {
	const float max = max(max(in.red, in.green), in.blue);
	const float min = min(min(in.red, in.green), in.blue);

	struct hsl out = { (max + min) / 2.0f };
	if (max == min ) {
		out.h = 0.0f;
		out.s = 0.0f;
	} else {
		const float delta = max - min;
		out.s = out.l > 0.5f ? delta / (2.0f - max - min) : delta / (max + min);
		if (max == in.red)
			out.h = (in.green - in.blue) / delta + (in.green < in.blue ? 6.0f : 0.0f);
		else if (max == in.green)
			out.h = (in.blue - in.red) / delta + 2.0f;
		else if (max == in.blue)
			out.h = (in.red - in.green) / delta + 4;

		out.h /= 6.0f;
	}
	return (struct hsl){ out.h * 360.0f, out.s * 100.0f, out.l * 100.0f };
}

struct color hsv_to_rgb(struct hsv in) {
	if (in.s <= 0.0f) return (struct color){ in.v, in.v, in.v, 1.0f };
	if (in.h >= 360.0f) in.h = 0.0f;
	in.h /= 60.0f;
	long segment = (long)in.h;
	float segment_offset = in.h - segment;
	float p = in.v * (1.0 - in.s);
	float q = in.v * (1.0 - (in.s * segment_offset));
	float t = in.v * (1.0 - (in.s * (1.0 - segment_offset)));

	switch(segment) {
	case 0: return (struct color){ in.v, t, p, 1.0f };
	case 1: return (struct color){ q, in.v, p, 1.0f };
	case 2: return (struct color){ p, in.v, t, 1.0f };
	case 3: return (struct color){ p, q, in.v, 1.0f };
	case 4: return (struct color){ t, p, in.v };
	case 5:
	default: return (struct color){ in.v, p, q };
	}
}

struct hsv rgb_to_hsv(struct color in) {
	const float max = max(max(in.red, in.green), in.blue);
	const float min = min(min(in.red, in.green), in.blue);
	struct hsv out = { .v = max };
	const float delta = max - min;
	if (delta < 0.00001f) {
		out.s = 0.0f;
		out.h = 0.0f;
		return out;
	}
	if (max > 0.0f) {
		out.s = delta / max;
	} else {
		// if max is 0, then r = g = b = 0
		// s = 0, h = NAN
		out.s = 0.0;
		out.h = NAN;
		return out;
	}
	if (in.red >= max)
		out.h = (in.green - in.blue) / delta;
	else if (in.green >= max)
		out.h = 2.0f + (in.blue - in.red) / delta;
	else
		out.h = 4.0f + (in.red - in.green) / delta;

	out.h *= 60.0f;

	if (out.h < 0.0f)
		out.h += 360.0f;

	return out;
}

void color_dump(struct color c, char *buf, size_t bufsize) {
	snprintf(buf, bufsize, "{ %.2f, %.2f, %.2f, %.2f }", (double)c.red, (double)c.green, (double)c.blue, (double)c.alpha);
}

