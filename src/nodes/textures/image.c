//
//  image.c
//  C-Ray
//
//  Created by Valtteri Koskivuori on 06/12/2020.
//  Copyright © 2020 Valtteri Koskivuori. All rights reserved.
//

#include "../../includes.h"
#include "../../datatypes/color.h"
#include "../../datatypes/poly.h"
#include "../../datatypes/vertexbuffer.h"
#include "../../utils/assert.h"
#include "../../datatypes/image/texture.h"
#include "../../utils/mempool.h"
#include "../../datatypes/hitrecord.h"
#include "../../utils/hashtable.h"
#include "../../datatypes/scene.h"
#include "texturenode.h"

#include "image.h"

struct imageTexture {
	struct textureNode node;
	struct texture *tex;
	uint8_t options;
};

//Transform the intersection coordinates to the texture coordinate space
//And grab the color at that point. Texture mapping.
struct color internalColor(const struct texture *tex, const struct hitRecord *isect, bool transform) {
	if (!tex) return warningMaterial().diffuse;
	if (!isect->material.texture) return warningMaterial().diffuse;
	if (!isect->polygon) return warningMaterial().diffuse;
	
	//Get the color value at these XY coordinates
	struct color output = textureGetPixel(tex, isect->uv.x, isect->uv.y, true);
	
	//Since the texture is probably srgb, transform it back to linear colorspace for rendering
	if (transform) output = fromSRGB(output);
	return output;
}

struct color evalTexture(const struct textureNode *node, const struct hitRecord *record) {
	// TODO: Consider transforming image textures while loading textures.
	// TODO: Handle NO_BILINEAR option
	struct imageTexture *image = (struct imageTexture *)node;
	return internalColor(image->tex, record, image->options & SRGB_TRANSFORM);
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

struct textureNode *newImageTexture(struct world *world, struct texture *texture, uint8_t options) {
	HASH_CONS(world->nodeTable, &world->nodePool, hash, struct imageTexture, {
		.tex = texture,
		.options = options,
		.node = {
			.eval = evalTexture,
			.base = { .compare = compare }
		}
	});
}
