//
//  texture.h
//  c-ray
//
//  Created by Valtteri Koskivuori on 09/04/2019.
//  Copyright © 2019-2022 Valtteri Koskivuori. All rights reserved.
//

#pragma once

#include <stdbool.h>
#include <stddef.h>
#include "color.h"
#include "dyn_array.h"

enum colorspace {
	linear,
	sRGB
};

enum precision {
	char_p,
	float_p,
	none
};

struct texture {
	enum colorspace colorspace;
	enum precision precision;
	union {
		unsigned char *byte_p; //For 24/32bit
		float *float_p; //For hdr
	} data;
	size_t channels;
	size_t width;
	size_t height;
};

struct texture_asset {
	char *path;
	struct texture *t;
};

typedef struct texture_asset texture_asset;
dyn_array_def(texture_asset)

//FIXME: These are opposite states, which is kinda confusing.
#define SRGB_TRANSFORM 0x01
#define NO_BILINEAR    0x02

struct texture *newTexture(enum precision p, size_t width, size_t height, size_t channels);

void setPixel(struct texture *t, struct color c, size_t x, size_t y);

/// Get a color value for a given pixel in a texture
/// @remarks When filtered == false, pass in the integer coordinates, otherwise pass in a 0.0f->1.0f coefficient
struct color textureGetPixel(const struct texture *t, float x, float y, bool filtered);

/// Convert texture from sRGB to linear color space
/// @remarks The texture data will be modified directly.
/// @param t Texture to convert
void textureFromSRGB(struct texture *t);

/// Convert texture from linear color space to sRGB.
/// @remarks The texture data will be modified directly.
/// @param t Texture to convert
void textureToSRGB(struct texture *t);

bool texture_uses_alpha(const struct texture *t);

void tex_clear(struct texture *t);

/// Deallocate a given texture
/// @param tex Texture to deallocate
void destroyTexture(struct texture *tex);
