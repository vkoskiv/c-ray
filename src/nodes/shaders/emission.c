//
//  emission.c
//  C-ray
//
//  Created by Valtteri on 2.1.2021.
//  Copyright © 2021-2022 Valtteri Koskivuori. All rights reserved.
//

#include "../../datatypes/color.h"
#include "../../renderer/samplers/sampler.h"
#include "../../datatypes/vector.h"
#include "../../datatypes/material.h"
#include "../colornode.h"
#include "../../utils/hashtable.h"
#include "../../datatypes/scene.h"
#include "../bsdfnode.h"

#include "emission.h"

struct emissiveBsdf {
	struct bsdfNode bsdf;
	const struct colorNode *color;
	const struct valueNode *strength;
};

static bool compare(const void *A, const void *B) {
	const struct emissiveBsdf *this = A;
	const struct emissiveBsdf *other = B;
	return this->color == other->color && this->strength == other->strength;
}

static uint32_t hash(const void *p) {
	const struct emissiveBsdf *this = p;
	uint32_t h = hashInit();
	h = hashBytes(h, &this->color, sizeof(this->color));
	h = hashBytes(h, &this->strength, sizeof(this->strength));
	return h;
}

static struct bsdfSample sample(const struct bsdfNode *bsdf, sampler *sampler, const struct hitRecord *record) {
	struct emissiveBsdf *emitBsdf = (struct emissiveBsdf *)bsdf;
	const struct vector scatterDir = vecNormalize(vecAdd(record->surfaceNormal, randomOnUnitSphere(sampler)));
	return (struct bsdfSample){
		.out = scatterDir,
		.color = colorCoef(emitBsdf->strength->eval(emitBsdf->strength, record), emitBsdf->color->eval(emitBsdf->color, record))
	};
}

const struct bsdfNode *newEmission(const struct node_storage *s, const struct colorNode *color, const struct valueNode *strength) {
	HASH_CONS(s->node_table, hash, struct emissiveBsdf, {
		.color = color ? color : newConstantTexture(s, blackColor),
		.strength = strength ? strength : newConstantValue(s, 1.0f),
		.bsdf = {
			.sample = sample,
			.base = { .compare = compare }
		}
	});
}
