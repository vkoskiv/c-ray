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
	
	const struct poly *p = isect->polygon;
	
	//Texture width and height for this material
	const float width = tex->width;
	const float heigh = tex->height;
	
	//barycentric coordinates for this polygon
	const float u = isect->uv.x;
	const float v = isect->uv.y;
	const float w = 1.0f - u - v;
	
	//Weighted texture coordinates
	const struct coord ucomponent = coordScale(u, g_textureCoords[p->textureIndex[1]]);
	const struct coord vcomponent = coordScale(v, g_textureCoords[p->textureIndex[2]]);
	const struct coord wcomponent = coordScale(w, g_textureCoords[p->textureIndex[0]]);
	
	// textureXY = u * v1tex + v * v2tex + w * v3tex
	const struct coord textureXY = addCoords(addCoords(ucomponent, vcomponent), wcomponent);
	
	const float x = (textureXY.x*(width));
	const float y = (textureXY.y*(heigh));
	
	//Get the color value at these XY coordinates
	struct color output = textureGetPixelFiltered(tex, x, y);
	
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

struct textureNode *newImageTexture(struct block **pool, struct texture *texture, uint8_t options) {
	struct imageTexture *new = allocBlock(pool, sizeof(*new));
	new->tex = texture;
	new->options = options;
	new->node.eval = evalTexture;
	return (struct textureNode *)new;
}
