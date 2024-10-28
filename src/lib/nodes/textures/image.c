//
//  image.c
//  c-ray
//
//  Created by Valtteri Koskivuori on 06/12/2020.
//  Copyright © 2020-2022 Valtteri Koskivuori. All rights reserved.
//

#include <stdio.h>
#include <string.h>
#include <common/color.h>
#include <common/texture.h>
#include <common/mempool.h>
#include <common/hashtable.h>
#include <datatypes/poly.h>
#include <datatypes/hitrecord.h>
#include <datatypes/scene.h>
#include "../colornode.h"

#include "image.h"

struct imageTexture {
	struct colorNode node;
	const struct texture *tex;
	uint8_t options;
};

//Transform the intersection coordinates to the texture coordinate space
//And grab the color at that point. Texture mapping.
static struct color internalColor(const struct texture *tex, const struct hitRecord *isect, uint8_t options) {
	if (unlikely(!tex->data.byte_p)) return g_pink_color; // Async img decode fails -> we may get an empty texture
	//Get the color value at these XY coordinates
	struct color output;
	if (options & NO_BILINEAR) {
		float x = isect->uv.x * tex->width;
		float y = isect->uv.y * tex->height;
		output = tex_get_px(tex, x, y, false);
	} else {
		output = tex_get_px(tex, isect->uv.x, isect->uv.y, true);
	}
	
	//Since the texture is probably srgb, transform it back to linear colorspace for rendering
	//FIXME: Why is this done during rendering?
	if (options & SRGB_TRANSFORM) output = colorFromSRGB(output);
	return output;
}

static bool compare(const void *A, const void *B) {
	const struct imageTexture *this = A;
	const struct imageTexture *other = B;
	return this->tex == other->tex;
}

static uint32_t hash(const void *p) {
	const struct imageTexture *this = p;
	uint32_t h = hashInit();
	h = hashBytes(h, &this->tex, sizeof(this->tex));
	h = hashBytes(h, &this->options, sizeof(this->options));
	return h;
}

static void dump(const void *node, char *dumpbuf, int len) {
	struct imageTexture *self = (struct imageTexture *)node;
	//TODO: Consider having imageTexture have a func to dump this.
	if (!self->tex) {
		snprintf(dumpbuf, len, "imageTexture { tex: null }");
		return;
	}
	if (!self->tex->data.byte_p) {
		snprintf(dumpbuf, len, "imageTexture { tex: pending }");
		return;
	}
	snprintf(dumpbuf, len, "imageTexture { tex: { %lux%lu, %lu channels, %s, %s }, options: %s %s }",
		self->tex->width,
		self->tex->height,
		self->tex->channels,
		self->tex->colorspace == linear ? "linear" : "sRGB",
		self->tex->precision == char_p ? "8 bits/channel" : "32 bits/channel",
		self->options & SRGB_TRANSFORM ? "SRGB_TRANSFORM" : "",
		self->options & NO_BILINEAR ? "NO_BILINEAR" : "");
}

static struct color eval(const struct colorNode *node, sampler *sampler, const struct hitRecord *record) {
	// TODO: Consider transforming image textures while loading textures.
	(void)sampler;
	struct imageTexture *image = (struct imageTexture *)node;
	return internalColor(image->tex, record, image->options);
}

const struct colorNode *newImageTexture(const struct node_storage *s, const struct texture *texture, uint8_t options) {
	if (!texture) return NULL;
	HASH_CONS(s->node_table, hash, struct imageTexture, {
		.tex = texture,
		.options = options,
		.node = {
			.eval = eval,
			.base = { .compare = compare, .dump = dump }
		}
	});
}
