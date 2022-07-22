//
//  isotropic.c
//  C-ray
//
//  Created by Valtteri on 27.5.2021.
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

#include "isotropic.h"

struct isotropicBsdf {
	struct bsdfNode bsdf;
	const struct colorNode *color;
};

static bool compare(const void *A, const void *B) {
	const struct isotropicBsdf *this = A;
	const struct isotropicBsdf *other = B;
	return this->color == other->color;
}

static uint32_t hash(const void *p) {
	const struct isotropicBsdf *this = p;
	uint32_t h = hashInit();
	h = hashBytes(h, &this->color, sizeof(this->color));
	return h;
}

static struct bsdfSample sample(const struct bsdfNode *bsdf, sampler *sampler, const struct hitRecord *record) {
	struct isotropicBsdf *isoBsdf = (struct isotropicBsdf *)bsdf;
	const struct vector scatterDir = vecNormalize(randomOnUnitSphere(sampler)); // Is this normalized already?
	return (struct bsdfSample){
		.out = scatterDir,
		.color = isoBsdf->color->eval(isoBsdf->color, record)
	};
}

const struct bsdfNode *newIsotropic(const struct node_storage *s, const struct colorNode *color) {
	HASH_CONS(s->node_table, hash, struct isotropicBsdf, {
		.color = color ? color : newConstantTexture(s, blackColor),
		.bsdf = {
			.sample = sample,
			.base = { .compare = compare }
		}
	});
}
