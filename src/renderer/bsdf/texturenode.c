//
//  texturenode.c
//  C-Ray
//
//  Created by Valtteri Koskivuori on 30/11/2020.
//  Copyright © 2020 Valtteri Koskivuori. All rights reserved.
//

#include "../../includes.h"
#include "../../datatypes/color.h"
#include "../samplers/sampler.h"
#include "../../datatypes/vector.h"
#include "../../datatypes/material.h"
#include "../../datatypes/image/texture.h"
#include "../../datatypes/vertexbuffer.h"
#include "../../datatypes/poly.h"
#include "bsdf.h"

#include "texturenode.h"

struct imageTexture {
	struct textureNode node;
	const struct texture *tex;
};

struct constantTexture {
	struct textureNode node;
	struct color color;
};

struct checkerTexture {
	struct textureNode node;
	const struct textureNode *colorA;
	const struct textureNode *colorB;
	float scale;
};

//Transform the intersection coordinates to the texture coordinate space
//And grab the color at that point. Texture mapping.
struct color internal_color(const struct texture *tex, const struct hitRecord *isect, bool transform) {
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
	// FIXME: This will transform non-color images too! (normal, spec)
	// TODO: Consider transforming image textures while loading textures.
	struct imageTexture *image = (struct imageTexture *)node;
	return internal_color(image->tex, record, true);
}

struct textureNode *newImageTexture(const struct texture *texture, uint8_t options) {
	(void)options;
	struct imageTexture *new = calloc(1, sizeof(*new));
	new->tex = texture;
	new->node.eval = evalTexture;
	return (struct textureNode *)new;
}

struct color evalConstant(const struct textureNode *node, const struct hitRecord *record) {
	(void)record;
	struct constantTexture *constant = (struct constantTexture *)node;
	return constant->color;
}

struct textureNode *newConstantTexture(struct color color) {
	struct constantTexture *new = calloc(1, sizeof(*new));
	new->color = color;
	new->node.eval = evalConstant;
	return (struct textureNode *)new;
}

// UV-mapped variant
static struct color mappedCheckerBoard(const struct hitRecord *isect, const struct textureNode *A, const struct textureNode *B, float coef) {
	ASSERT(isect->material.texture);
	const struct poly *p = isect->polygon;
	
	//barycentric coordinates for this polygon
	const float u = isect->uv.x;
	const float v = isect->uv.y;
	const float w = 1.0f - u - v;
	
	//Weighted coordinates
	const struct coord ucomponent = coordScale(u, g_textureCoords[p->textureIndex[1]]);
	const struct coord vcomponent = coordScale(v, g_textureCoords[p->textureIndex[2]]);
	const struct coord wcomponent = coordScale(w, g_textureCoords[p->textureIndex[0]]);
	
	// textureXY = u * v1tex + v * v2tex + w * v3tex
	const struct coord surfaceXY = addCoords(addCoords(ucomponent, vcomponent), wcomponent);
	
	const float sines = sinf(coef*surfaceXY.x) * sinf(coef*surfaceXY.y);
	
	if (sines < 0.0f) {
		return A->eval(A, isect);
	} else {
		return B->eval(B, isect);
	}
}

// Fallback axis-aligned checkerboard
static struct color unmappedCheckerBoard(const struct hitRecord *isect, const struct textureNode *A, const struct textureNode *B, float coef) {
	const float sines = sinf(coef*isect->hitPoint.x) * sinf(coef*isect->hitPoint.y) * sinf(coef*isect->hitPoint.z);
	if (sines < 0.0f) {
		return A->eval(A, isect);
	} else {
		return B->eval(B, isect);
	}
}

static struct color checkerBoard(const struct hitRecord *isect, const struct textureNode *A, const struct textureNode *B, float scale) {
	return isect->material.texture ? mappedCheckerBoard(isect, A, B, scale) : unmappedCheckerBoard(isect, A, B, scale);
}

struct color evalCheckerboard(const struct textureNode *node, const struct hitRecord *record) {
	struct checkerTexture *checker = (struct checkerTexture *)node;
	return checkerBoard(record, checker->colorA, checker->colorB, checker->scale);
}

struct textureNode *newColorCheckerBoardTexture(const struct textureNode *colorA, const struct textureNode *colorB, float size) {
	struct checkerTexture *new = calloc(1, sizeof(*new));
	new->colorA = colorA;
	new->colorB = colorB;
	new->scale = size;
	new->node.eval = evalCheckerboard;
	return (struct textureNode *)new;
}

struct textureNode *newCheckerBoardTexture(float size) {
	struct checkerTexture *new = calloc(1, sizeof(*new));
	new->colorA = newConstantTexture(blackColor);
	new->colorB = newConstantTexture(whiteColor);
	new->scale = size;
	new->node.eval = evalCheckerboard;
	return (struct textureNode *)new;
}
