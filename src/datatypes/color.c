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
#include "../vendored/cJSON.h"
#include "../utils/assert.h"

//Some standard colours
const struct color g_red_color =   { 1.0f, 0.0f, 0.0f, 1.0f };
const struct color g_green_color = { 0.0f, 1.0f, 0.0f, 1.0f };
const struct color g_blue_color =  { 0.0f, 0.0f, 1.0f, 1.0f };
const struct color g_black_color = { 0.0f, 0.0f, 0.0f, 1.0f };
const struct color g_gray_color =  { 0.5f, 0.5f, 0.5f, 1.0f };
const struct color g_white_color = { 1.0f, 1.0f, 1.0f, 1.0f };
const struct color g_clear_color = { 0.0f, 0.0f, 0.0f, 0.0f };

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
static float hue_to_rgb(float p, float q, float t) {
	if (t < 0.0f) t += 1.0f;
	if (t > 1.0f) t -= 1.0f;
	if (t < 1.0f / 6.0f) return p + (q - p) * 6.0f * t;
	if (t < 1.0f / 2.0f) return q;
	if (t < 2.0f / 3.0f) return p + (q - p) * (2.0f / 3.0f - t) * 6.0f;
	return p;
}

struct color color_from_hsl(float hue, float saturation, float lightness) {
	// Map these to HSL standard ranges. 0-360 for h, 0-100 for s and l
	hue = hue / 360.0f;
	saturation = saturation / 100.0f;
	lightness = lightness / 100.0f;
	if (saturation == 0.0f) {
		return (struct color){ lightness, lightness, lightness, 1.0f };
	} else {
		const float q = lightness < 0.5f ? lightness * (1.0f + saturation) : lightness + saturation - lightness * saturation;
		const float p = 2.0f * lightness - q;
		return (struct color){ hue_to_rgb(p, q, hue + 1.0f / 3.0f), hue_to_rgb(p, q, hue), hue_to_rgb(p, q, hue - 1.0f / 3.0f), 1.0f };
	}
}

void color_dump(struct color c, char *buf, size_t bufsize) {
	snprintf(buf, bufsize, "{ %.2f, %.2f, %.2f, %.2f }", (double)c.red, (double)c.green, (double)c.blue, (double)c.alpha);
}

struct color color_parse(const cJSON *data) {
	if (cJSON_IsArray(data)) {
		const float r = cJSON_IsNumber(cJSON_GetArrayItem(data, 0)) ? (float)cJSON_GetArrayItem(data, 0)->valuedouble : 0.0f;
		const float g = cJSON_IsNumber(cJSON_GetArrayItem(data, 1)) ? (float)cJSON_GetArrayItem(data, 1)->valuedouble : 0.0f;
		const float b = cJSON_IsNumber(cJSON_GetArrayItem(data, 2)) ? (float)cJSON_GetArrayItem(data, 2)->valuedouble : 0.0f;
		const float a = cJSON_IsNumber(cJSON_GetArrayItem(data, 3)) ? (float)cJSON_GetArrayItem(data, 3)->valuedouble : 1.0f;
		return (struct color){ r, g, b, a };
	}

	ASSERT(cJSON_IsObject(data));

	const cJSON *kelvin = cJSON_GetObjectItem(data, "blackbody");
	if (cJSON_IsNumber(kelvin)) return colorForKelvin(kelvin->valuedouble);

	const cJSON *H = cJSON_GetObjectItem(data, "h");
	const cJSON *S = cJSON_GetObjectItem(data, "s");
	const cJSON *L = cJSON_GetObjectItem(data, "l");

	if (cJSON_IsNumber(H) && cJSON_IsNumber(S) && cJSON_IsNumber(L)) {
		return color_from_hsl(H->valuedouble, S->valuedouble, L->valuedouble);
	}

	const cJSON *R = cJSON_GetObjectItem(data, "r");
	const cJSON *G = cJSON_GetObjectItem(data, "g");
	const cJSON *B = cJSON_GetObjectItem(data, "b");
	const cJSON *A = cJSON_GetObjectItem(data, "a");

	return (struct color){
		cJSON_IsNumber(R) ? (float)R->valuedouble : 0.0f,
		cJSON_IsNumber(G) ? (float)G->valuedouble : 0.0f,
		cJSON_IsNumber(B) ? (float)B->valuedouble : 0.0f,
		cJSON_IsNumber(A) ? (float)A->valuedouble : 1.0f,
	};
}
