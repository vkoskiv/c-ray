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

struct color mix_color(const struct hitRecord *record) {
	return record->material.texture ? colorForUV(record, Diffuse) : record->material.diffuse;
}

struct bsdfSample mix_sample(const struct bsdf *bsdf, sampler *sampler, const struct hitRecord *record, const struct vector *in) {
	(void)in;
	struct mixBsdf *mixBsdf = (struct mixBsdf*)bsdf;
	const struct vector scatterDir = vecNormalize(vecAdd(record->surfaceNormal, randomOnUnitSphere(sampler)));
	return (struct bsdfSample){.out = scatterDir, .color = mixBsdf->eval(record)};
}

struct mixBsdf *newMix() {
	struct mixBsdf *new = calloc(1, sizeof(*new));
	new->eval = mix_color;
	new->bsdf.sample = mix_sample;
	return new;
}
