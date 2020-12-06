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
#include "../textures/texturenode.h"
#include "bsdf.h"

#include "diffuse.h"

struct diffuseBsdf {
	struct bsdf bsdf;
	struct textureNode *color;
};

struct bsdfSample sampleDiffuse(const struct bsdf *bsdf, sampler *sampler, const struct hitRecord *record) {
	struct diffuseBsdf *diffBsdf = (struct diffuseBsdf*)bsdf;
	const struct vector scatterDir = vecNormalize(vecAdd(record->surfaceNormal, randomOnUnitSphere(sampler)));
	return (struct bsdfSample){.out = scatterDir, .color = diffBsdf->color->eval(diffBsdf->color, record)};
}

struct bsdf *newDiffuse(struct block **pool, struct textureNode *tex) {
	struct diffuseBsdf *new = allocBlock(pool, sizeof(*new));
	new->color = tex;
	new->bsdf.sample = sampleDiffuse;
	return (struct bsdf *)new;
}
