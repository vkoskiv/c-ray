//
//  translucent.c
//  C-Ray
//
//  Created by Valtteri Koskivuori on 05/06/2022.
//  Copyright © 2022 Valtteri Koskivuori. All rights reserved.
//

#include "../../datatypes/color.h"
#include "../../renderer/samplers/sampler.h"
#include "../../datatypes/vector.h"
#include "../../datatypes/material.h"
#include "../colornode.h"
#include "../../utils/hashtable.h"
#include "../../datatypes/scene.h"
#include "../bsdfnode.h"

#include "translucent.h"

struct translucentBsdf {
	struct bsdfNode bsdf;
	const struct colorNode *color;
};

static bool compare(const void *A, const void *B) {
	const struct translucentBsdf *this = A;
	const struct translucentBsdf *other = B;
	return this->color == other->color;
}

static uint32_t hash(const void *p) {
	const struct translucentBsdf *this = p;
	uint32_t h = hashInit();
	h = hashBytes(h, &this->color, sizeof(this->color));
	return h;
}

static struct bsdfSample sample(const struct bsdfNode *bsdf, sampler *sampler, const struct hitRecord *record) {
	struct translucentBsdf *diffBsdf = (struct translucentBsdf *)bsdf;
	const struct vector scatterDir = vecNormalize(vecAdd(vecNegate(record->surfaceNormal), randomOnUnitSphere(sampler)));
	return (struct bsdfSample){
			.out = scatterDir,
			.color = diffBsdf->color->eval(diffBsdf->color, record)
	};
}

const struct bsdfNode *newTranslucent(const struct node_storage *s, const struct colorNode *color) {
	HASH_CONS(s->node_table, hash, struct translucentBsdf, {
		.color = color ? color : newConstantTexture(s, blackColor),
		.bsdf = {
				.sample = sample,
				.base = { .compare = compare }
		}
	});
}