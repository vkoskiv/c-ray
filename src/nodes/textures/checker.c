//
//  checker.c
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
#include "texturenode.h"

#include "checker.h"

struct checkerTexture {
	struct textureNode node;
	struct textureNode *colorA;
	struct textureNode *colorB;
	float scale;
};

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

static bool compare(const void *A, const void *B) {
	const struct checkerTexture *this = A;
	const struct checkerTexture *other = B;
	return this->colorA == other->colorA && this->colorB == other->colorB && this->scale == other->scale;
}

static uint32_t hash(const void *p) {
	const struct checkerTexture *this = p;
	uint32_t h = hashInit();
	h = hashBytes(h, &this->colorA, sizeof(this->colorA));
	h = hashBytes(h, &this->colorB, sizeof(this->colorB));
	h = hashBytes(h, &this->scale, sizeof(this->scale));
	return h;
}

struct textureNode *newColorCheckerBoardTexture(struct block **pool, struct textureNode *colorA, struct textureNode *colorB, float size) {
	struct checkerTexture *new = allocBlock(pool, sizeof(*new));
	new->colorA = colorA;
	new->colorB = colorB;
	new->scale = size;
	new->node.eval = evalCheckerboard;
	new->node.base.compare = compare;
	new->node.base.hash = hash;
	return (struct textureNode *)new;
}

struct textureNode *newCheckerBoardTexture(struct block **pool, float size) {
	struct checkerTexture *new = allocBlock(pool, sizeof(*new));
	new->colorA = newConstantTexture(pool, blackColor);
	new->colorB = newConstantTexture(pool, whiteColor);
	new->scale = size;
	new->node.eval = evalCheckerboard;
	new->node.base.compare = compare;
	new->node.base.hash = hash;
	return (struct textureNode *)new;
}
