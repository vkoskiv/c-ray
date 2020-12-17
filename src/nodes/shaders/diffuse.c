//
//  diffuse.c
//  C-Ray
//
//  Created by Valtteri Koskivuori on 30/11/2020.
//  Copyright © 2020 Valtteri Koskivuori. All rights reserved.
//

#include "../../includes.h"
#include "../../datatypes/color.h"
#include "../../renderer/samplers/sampler.h"
#include "../../datatypes/vector.h"
#include "../../datatypes/material.h"
#include "../colornode.h"
#include "../../utils/hashtable.h"
#include "../../datatypes/scene.h"
#include "../../utils/logging.h"
#include "../bsdfnode.h"

#include "diffuse.h"

struct diffuseBsdf {
	struct bsdfNode bsdf;
	struct colorNode *color;
};

struct bsdfSample sampleDiffuse(const struct bsdfNode *bsdf, sampler *sampler, const struct hitRecord *record) {
	struct diffuseBsdf *diffBsdf = (struct diffuseBsdf*)bsdf;
	const struct vector scatterDir = vecNormalize(vecAdd(record->surfaceNormal, randomOnUnitSphere(sampler)));
	return (struct bsdfSample){
		.out = scatterDir,
		.color = diffBsdf->color ? diffBsdf->color->eval(diffBsdf->color, record) : record->material.diffuse
	};
}

static bool compare(const void *A, const void *B) {
	const struct diffuseBsdf *this = A;
	const struct diffuseBsdf *other = B;
	return this->color == other->color;
}

static uint32_t hash(const void *p) {
	const struct diffuseBsdf *this = p;
	uint32_t h = hashInit();
	h = hashBytes(h, &this->color, sizeof(this->color));
	return h;
}

struct bsdfNode *newDiffuse(struct world *world, struct colorNode *tex) {
	HASH_CONS(world->nodeTable, &world->nodePool, hash, struct diffuseBsdf, {
		.color = tex ? tex : newConstantTexture(world, blackColor),
		.bsdf = {
			.sample = sampleDiffuse,
			.base = { .compare = compare }
		}
	});
}
