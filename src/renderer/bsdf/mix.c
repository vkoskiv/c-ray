//
//  mix.c
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
#include "bsdf.h"

#include "mix.h"

struct mixBsdf {
	struct bsdf bsdf;
	struct bsdf *A;
	struct bsdf *B;
	struct textureNode *lerp;
};

struct bsdfSample sampleMix(const struct bsdf *bsdf, sampler *sampler, const struct hitRecord *record, const struct vector *in) {
	struct mixBsdf *mixBsdf = (struct mixBsdf*)bsdf;
	//TODO: Do we need grayscale()?
	const float lerp = mixBsdf->lerp ? grayscale(mixBsdf->lerp->eval(mixBsdf->lerp, record)).red : 0.5f;
	if (getDimension(sampler) < lerp) {
		return mixBsdf->A->sample(mixBsdf->A, sampler, record, in);
	} else {
		return mixBsdf->B->sample(mixBsdf->B, sampler, record, in);
	}
}

struct bsdf *newMix(struct bsdf *A, struct bsdf *B, struct textureNode *lerp) {
	ASSERT(A);
	ASSERT(B);
	ASSERT(lerp);
	struct mixBsdf *new = calloc(1, sizeof(*new));
	new->lerp = lerp;
	new->A = A;
	new->B = B;
	new->bsdf.sample = sampleMix;
	new->bsdf.destroy = destroyMix;
	return (struct bsdf *)new;
}

struct constantMixBsdf {
	struct bsdf bsdf;
	struct bsdf *A;
	struct bsdf *B;
	float mix;
};

struct bsdfSample sampleConstantMix(const struct bsdf *bsdf, sampler *sampler, const struct hitRecord *record, const struct vector *in) {
	struct constantMixBsdf *mixBsdf = (struct constantMixBsdf*)bsdf;
	//TODO: Do we need grayscale()?
	if (getDimension(sampler) < mixBsdf->mix) {
		return mixBsdf->A->sample(mixBsdf->A, sampler, record, in);
	} else {
		return mixBsdf->B->sample(mixBsdf->B, sampler, record, in);
	}
}

struct bsdf *newMixConstant(struct bsdf *A, struct bsdf *B, float mix) {
	struct constantMixBsdf *new = calloc(1, sizeof(*new));
	new->A = A;
	new->B = B;
	new->mix = mix;
	new->bsdf.sample = sampleConstantMix;
	new->bsdf.destroy = destroyMix;
	return (struct bsdf *)new;
}

void destroyMix(struct bsdf *bsdf) {
	struct mixBsdf *mix = (struct mixBsdf *)bsdf;
	destroyBsdf(mix->A);
	destroyBsdf(mix->B);
	destroyTextureNode(mix->lerp);
	free(mix);
}
