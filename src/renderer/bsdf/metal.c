//
//  metal.c
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

#include "metal.h"

struct metalBsdf {
	struct bsdf bsdf;
	struct textureNode *color;
	struct textureNode *roughness;
};

struct bsdfSample metal_sample(const struct bsdf *bsdf, sampler *sampler, const struct hitRecord *record, const struct vector *in) {
	(void)in;
	struct metalBsdf *metalBsdf = (struct metalBsdf*)bsdf;
	
	const struct vector normalizedDir = vecNormalize(*in);
	struct vector reflected = reflectVec(&normalizedDir, &record->surfaceNormal);
	float roughness = metalBsdf->roughness->eval(metalBsdf->roughness, record).red;
	if (roughness > 0.0f) {
		const struct vector fuzz = vecScale(randomOnUnitSphere(sampler), roughness);
		reflected = vecAdd(reflected, fuzz);
	}
	
	return (struct bsdfSample){.out = reflected, .color = metalBsdf->color->eval(metalBsdf->color, record)};
}

struct bsdf *newMetal(struct textureNode *color, struct textureNode *roughness) {
	struct metalBsdf *new = calloc(1, sizeof(*new));
	new->color = color;
	new->roughness = roughness;
	new->bsdf.sample = metal_sample;
	return (struct bsdf *)new;
}
