//
//  mix.c
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
#include "../../utils/hashtable.h"
#include "../../datatypes/scene.h"
#include "bsdf.h"

#include "mix.h"

struct mixBsdf {
	struct bsdf bsdf;
	struct bsdf *A;
	struct bsdf *B;
	struct textureNode *lerp;
};

struct bsdfSample sampleMix(const struct bsdf *bsdf, sampler *sampler, const struct hitRecord *record) {
	struct mixBsdf *mixBsdf = (struct mixBsdf*)bsdf;
	//TODO: Do we need grayscale()?
	const float lerp = mixBsdf->lerp ? grayscale(mixBsdf->lerp->eval(mixBsdf->lerp, record)).red : 0.5f;
	if (getDimension(sampler) < lerp) {
		return mixBsdf->A->sample(mixBsdf->A, sampler, record);
	} else {
		return mixBsdf->B->sample(mixBsdf->B, sampler, record);
	}
}

static bool compareMix(const void *A, const void *B) {
	struct mixBsdf *this = (struct mixBsdf *)A;
	struct mixBsdf *other = (struct mixBsdf *)B;
	return this->A == other->A && this->B == other->B && this->lerp == other->lerp;
}

static uint32_t hashMix(const void *p) {
	const struct mixBsdf *this = p;
	uint32_t h = hashInit();
	h = hashBytes(h, &this->A, sizeof(this->A));
	h = hashBytes(h, &this->B, sizeof(this->B));
	h = hashBytes(h, &this->lerp, sizeof(this->lerp));
	return h;
}

struct bsdf *newMix(struct world *world, struct bsdf *A, struct bsdf *B, struct textureNode *lerp) {
	HASH_CONS(world->nodeTable, &world->nodePool, hashMix, struct mixBsdf, {
		.A = A,
		.B = B,
		.lerp = lerp,
		.bsdf = {
			.sample = sampleMix,
			.base = { .compare = compareMix }
		}
	});
}

struct constantMixBsdf {
	struct bsdf bsdf;
	struct bsdf *A;
	struct bsdf *B;
	float mix;
};

struct bsdfSample sampleConstantMix(const struct bsdf *bsdf, sampler *sampler, const struct hitRecord *record) {
	struct constantMixBsdf *mixBsdf = (struct constantMixBsdf*)bsdf;
	if (getDimension(sampler) < mixBsdf->mix) {
		return mixBsdf->A->sample(mixBsdf->A, sampler, record);
	} else {
		return mixBsdf->B->sample(mixBsdf->B, sampler, record);
	}
}

static bool compareConstant(const void *A, const void *B) {
	const struct constantMixBsdf *this = A;
	const struct constantMixBsdf *other = B;
	return this->mix == other->mix && this->A->base.compare(this->A, other->A) && this->B->base.compare(this->B, other->B);
}

static uint32_t hashConstant(const void *p) {
	const struct constantMixBsdf *this = p;
	uint32_t h = hashInit();
	h = hashBytes(h, &this->A, sizeof(this->A));
	h = hashBytes(h, &this->B, sizeof(this->B));
	h = hashBytes(h, &this->mix, sizeof(this->mix));
	return h;
}

struct bsdf *newMixConstant(struct world *world, struct bsdf *A, struct bsdf *B, float mix) {
	HASH_CONS(world->nodeTable, &world->nodePool, hashConstant, struct constantMixBsdf, {
		.A = A,
		.B = B,
		.mix = mix,
		.bsdf = {
			.sample = sampleConstantMix,
			.base = { .compare = compareConstant }
		}
	});
}
