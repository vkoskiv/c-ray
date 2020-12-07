//
//  metal.c
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
#include "bsdf.h"

#include "metal.h"

struct metalBsdf {
	struct bsdf bsdf;
	struct textureNode *color;
	struct textureNode *roughness;
};

struct bsdfSample sampleMetal(const struct bsdf *bsdf, sampler *sampler, const struct hitRecord *record) {
	struct metalBsdf *metalBsdf = (struct metalBsdf*)bsdf;
	
	const struct vector normalizedDir = vecNormalize(record->incident.direction);
	struct vector reflected = reflectVec(&normalizedDir, &record->surfaceNormal);
	float roughness = metalBsdf->roughness->eval(metalBsdf->roughness, record).red;
	if (roughness > 0.0f) {
		const struct vector fuzz = vecScale(randomOnUnitSphere(sampler), roughness);
		reflected = vecAdd(reflected, fuzz);
	}
	
	return (struct bsdfSample){
		.out = reflected,
		.color = metalBsdf->color ? metalBsdf->color->eval(metalBsdf->color, record) : record->material.diffuse
	};
}

struct bsdf *newMetal(struct block **pool, struct textureNode *color, struct textureNode *roughness) {
	struct metalBsdf *new = allocBlock(pool, sizeof(*new));
	new->color = color;
	new->roughness = roughness;
	new->bsdf.sample = sampleMetal;
	return (struct bsdf *)new;
}
