//
//  image.c
//  C-Ray
//
//  Created by Valtteri Koskivuori on 06/12/2020.
//  Copyright © 2020-2022 Valtteri Koskivuori. All rights reserved.
//

#include "../../datatypes/color.h"
#include "../../datatypes/poly.h"
#include "../../datatypes/image/texture.h"
#include "../../utils/mempool.h"
#include "../../datatypes/hitrecord.h"
#include "../../utils/hashtable.h"
#include "../../datatypes/scene.h"
#include "../colornode.h"

#include "image.h"

struct imageTexture {
	struct colorNode node;
	const struct texture *tex;
	uint8_t options;
};

//Transform the intersection coordinates to the texture coordinate space
//And grab the color at that point. Texture mapping.
struct color internalColor(const struct texture *tex, const struct hitRecord *isect, uint8_t options) {
	if (!tex) return warningMaterial().diffuse;
	
	//Get the color value at these XY coordinates
	struct color output;
	if (options & NO_BILINEAR) {
		float x = isect->uv.x * tex->width;
		float y = isect->uv.y * tex->height;
		output = textureGetPixel(tex, x, y, false);
	} else {
		output = textureGetPixel(tex, isect->uv.x, isect->uv.y, true);
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

static struct color eval(const struct colorNode *node, const struct hitRecord *record) {
	// TODO: Consider transforming image textures while loading textures.
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
			.base = { .compare = compare }
		}
	});
}
