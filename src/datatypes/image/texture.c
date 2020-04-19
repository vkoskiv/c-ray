//
//  texture.c
//  C-ray
//
//  Created by Valtteri on 09/04/2019.
//  Copyright © 2015-2020 Valtteri Koskivuori. All rights reserved.
//

#include "../../includes.h"

#include "texture.h"
#include "../color.h"
#include "../../utils/logging.h"
#include "../../utils/assert.h"

//General-purpose blit function
void blit(struct texture *t, struct color c, unsigned x, unsigned y) {
	ASSERT(x < t->width); ASSERT(y < t->height);
	if (t->precision == char_p) {
		t->data.byte_p[(x + (t->height - (y + 1)) * t->width) * t->channels + 0] = (unsigned char)min(c.red * 255.0f, 255.0f);
		t->data.byte_p[(x + (t->height - (y + 1)) * t->width) * t->channels + 1] = (unsigned char)min(c.green * 255.0f, 255.0f);
		t->data.byte_p[(x + (t->height - (y + 1)) * t->width) * t->channels + 2] = (unsigned char)min(c.blue * 255.0f, 255.0f);
		if (t->hasAlpha) t->data.byte_p[(x + (t->height - (y + 1)) * t->width) * t->channels + 3] = (unsigned char)min(c.alpha * 255.0f, 255.0f);
	}
	else if (t->precision == float_p) {
		t->data.float_p[(x + (t->height - (y + 1)) * t->width) * t->channels + 0] = c.red;
		t->data.float_p[(x + (t->height - (y + 1)) * t->width) * t->channels + 1] = c.green;
		t->data.float_p[(x + (t->height - (y + 1)) * t->width) * t->channels + 2] = c.blue;
		if (t->hasAlpha) t->data.float_p[(x + (t->height - (y + 1)) * t->width) * t->channels + 3] = c.alpha;
	}
}

struct color textureGetPixel(const struct texture *t, unsigned x, unsigned y) {
	struct color output = {0.0f, 0.0f, 0.0f, 0.0f};
	int pitch = 0;
	if (t->hasAlpha) {
		pitch = 4;
	} else {
		pitch = 3;
	}
	
	//bilinear lerp might tweak the values, so just clamp here to be safe.
	x = x > t->width-1 ? t->width-1 : x;
	y = y > t->height-1 ? t->height-1 : y;
	
	if (t->precision == float_p) {
		output.red = t->data.float_p[(x + ((t->height-1) - y) * t->width)*pitch + 0];
		output.green = t->data.float_p[(x + ((t->height-1) - y) * t->width)*pitch + 1];
		output.blue = t->data.float_p[(x + ((t->height-1) - y) * t->width)*pitch + 2];
		output.alpha = t->hasAlpha ? t->data.float_p[(x + ((t->height-1) - y) * t->width)*pitch + 3] : 1.0f;
	} else {
		output.red = t->data.byte_p[(x + ((t->height-1) - y) * t->width)*pitch + 0] / 255.0f;
		output.green = t->data.byte_p[(x + ((t->height-1) - y) * t->width)*pitch + 1] / 255.0f;
		output.blue = t->data.byte_p[(x + ((t->height-1) - y) * t->width)*pitch + 2] / 255.0f;
		output.alpha = t->hasAlpha ? t->data.byte_p[(x + (t->height - y) * t->width)*pitch + 3] / 255.0f : 1.0f;
	}
	
	return output;
}

//Bilinearly interpolated (smoothed) output. Requires float precision, i.e. 0.0->width-1.0
struct color textureGetPixelFiltered(const struct texture *t, float x, float y) {
	float xcopy = x - 0.5f;
	float ycopy = y - 0.5f;
	int xint = (int)xcopy;
	int yint = (int)ycopy;
	struct color topleft = textureGetPixel(t, xint, yint);
	struct color topright = textureGetPixel(t, xint + 1, yint);
	struct color botleft = textureGetPixel(t, xint, yint + 1);
	struct color botright = textureGetPixel(t, xint + 1, yint + 1);
	return lerp(lerp(topleft, topright, xcopy-xint), lerp(botleft, botright, xcopy-xint), ycopy-yint);
}

struct texture *newTexture(enum precision p, unsigned width, unsigned height, int channels) {
	struct texture *t = calloc(1, sizeof(struct texture));
	t->width = width;
	t->height = height;
	t->precision = p;
	t->channels = channels;
	t->hasAlpha = false;
	t->data.byte_p = NULL;
	t->data.float_p = NULL;
	t->colorspace = linear;
	if (channels > 3) {
		t->hasAlpha = true;
	}
	
	switch (t->precision) {
		case char_p: {
			t->data.byte_p = calloc(channels * width * height, sizeof(unsigned char));
			if (!t->data.byte_p) {
				logr(warning, "Failed to allocate %ix%i texture.\n", width, height);
				destroyTexture(t);
				return NULL;
			}
		}
			break;
		case float_p: {
			t->data.float_p = calloc(channels * width * height, sizeof(float));
			if (!t->data.float_p) {
				logr(warning, "Failed to allocate %ix%i texture.\n", width, height);
				destroyTexture(t);
				return NULL;
			}
		}
			break;
		default:
			break;
	}
	return t;
}

void textureFromSRGB(struct texture *t) {
	if (t->colorspace == sRGB) return;
	for (unsigned x = 0; x < t->width; ++x) {
		for (unsigned y = 0; y < t->height; ++y) {
			blit(t, fromSRGB(textureGetPixel(t, x, y)), x, y);
		}
	}
	t->colorspace = linear;
}

void textureToSRGB(struct texture *t) {
	if (t->colorspace == linear) return;
	for (unsigned x = 0; x < t->width; ++x) {
		for (unsigned y = 0; y < t->height; ++y) {
			blit(t, toSRGB(textureGetPixel(t, x, y)), x, y);
		}
	}
	t->colorspace = sRGB;
}

void destroyTexture(struct texture *t) {
	if (t) {
		if (t->data.byte_p) { //Union, will also free float_p
			free(t->data.byte_p);
		}
		free(t);
	}
}
