//
//  checker.c
//  c-ray
//
//  Created by Valtteri Koskivuori on 06/12/2020.
//  Copyright © 2020-2025 Valtteri Koskivuori. All rights reserved.
//

#include <stdio.h>
#include <common/color.h>
#include <common/texture.h>
#include <common/mempool.h>
#include <common/hashtable.h>
#include <datatypes/poly.h>
#include <datatypes/hitrecord.h>
#include <datatypes/scene.h>
#include "../colornode.h"

#include "checker.h"

struct checkerTexture {
	struct colorNode node;
	const struct colorNode *A;
	const struct colorNode *B;
	const struct valueNode *scale;
};

// UV-mapped variant
static struct color mappedCheckerBoard(const struct hitRecord *isect, sampler *sampler, const struct colorNode *A, const struct colorNode *B, const struct valueNode *scale) {
	const float coef = scale->eval(scale, sampler, isect);
	const coord uv = coord_scale(coef, isect->uv);
	float x_i = (uv.x + 0.000001) * 0.999999;
	float y_i = (uv.y + 0.000001) * 0.999999;
	x_i = (int)fabsf(floorf(x_i));
	y_i = (int)fabsf(floorf(y_i));
	if (fmodf(x_i, 2.0f) == fmodf(y_i, 2.0f)) {
		return A->eval(A, sampler, isect);
	} else {
		return B->eval(B, sampler, isect);
	}
}

// Fallback axis-aligned checkerboard
static struct color unmappedCheckerBoard(const struct hitRecord *isect, sampler *sampler, const struct colorNode *A, const struct colorNode *B, const struct valueNode *scale) {
	const float coef = scale->eval(scale, sampler, isect);
	const vector v = vec_scale(isect->hitPoint, coef);
	float x_i = (v.x + 0.000001) * 0.999999;
	float y_i = (v.y + 0.000001) * 0.999999;
	float z_i = (v.z + 0.000001) * 0.999999;
	x_i = (int)fabsf(floorf(x_i));
	y_i = (int)fabsf(floorf(y_i));
	z_i = (int)fabsf(floorf(z_i));
	if ((fmodf(x_i, 2.0f) == fmodf(y_i, 2.0f)) == fmodf(z_i, 2.0f)) {
		return A->eval(A, sampler, isect);
	} else {
		return B->eval(B, sampler, isect);
	}
}

static struct color checkerBoard(const struct hitRecord *isect, sampler *sampler, const struct colorNode *A, const struct colorNode *B, const struct valueNode *scale) {
	return isect->uv.x >= 0 ? mappedCheckerBoard(isect, sampler, A, B, scale) : unmappedCheckerBoard(isect, sampler, A, B, scale);
}

static bool compare(const void *A, const void *B) {
	const struct checkerTexture *this = A;
	const struct checkerTexture *other = B;
	return this->A == other->A && this->B == other->B && this->scale == other->scale;
}

static uint32_t hash(const void *p) {
	const struct checkerTexture *this = p;
	uint32_t h = hashInit();
	h = hashBytes(h, &this->A, sizeof(this->A));
	h = hashBytes(h, &this->B, sizeof(this->B));
	h = hashBytes(h, &this->scale, sizeof(this->scale));
	return h;
}

static void dump(const void *node, char *dumpbuf, int len) {
	struct checkerTexture *self = (struct checkerTexture *)node;
	char A[64] = "";
	if (self->A->base.dump) self->A->base.dump(self->A, &A[0], 64);
	char B[64] = "";
	if (self->B->base.dump) self->B->base.dump(self->B, &B[0], 64);
	snprintf(dumpbuf, len, "checkerTexture { A: %s, B: %s }", A, B);
}

static struct color eval(const struct colorNode *node, sampler *sampler, const struct hitRecord *record) {
	struct checkerTexture *checker = (struct checkerTexture *)node;
	return checkerBoard(record, sampler, checker->A, checker->B, checker->scale);
}

//TODO: Maybe a 'local' flag that would then remap UVs to be local to each checker square? That'd be neat. Blender doesn't have it.
const struct colorNode *newCheckerBoardTexture(const struct node_storage *s, const struct colorNode *A, const struct colorNode *B, const struct valueNode *scale) {
	HASH_CONS(s->node_table, hash, struct checkerTexture, {
		.A = A ? A : newConstantTexture(s, g_black_color),
		.B = B ? B : newConstantTexture(s, g_white_color),
		.scale = scale ? scale : newConstantValue(s, 5.0f),
		.node = {
			.eval = eval,
			.base = { .compare = compare, .dump = dump }
		}
	});
}
