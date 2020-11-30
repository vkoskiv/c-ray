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

struct color metal_color(const struct hitRecord *record) {
	return record->material.texture ? colorForUV(record, Diffuse) : record->material.diffuse;
}

struct bsdfSample metal_sample(const struct bsdf *bsdf, sampler *sampler, const struct hitRecord *record, const struct vector *in) {
	(void)in;
	struct metalBsdf *metalBsdf = (struct metalBsdf*)bsdf;
	
	const struct vector normalizedDir = vecNormalize(record->incident.direction);
	struct vector reflected = reflectVec(&normalizedDir, &record->surfaceNormal);
	//Roughness
	float roughness = record->material.specularMap ? colorForUV(record, Specular).red : record->material.roughness;
	if (roughness > 0.0f) {
		const struct vector fuzz = vecScale(randomOnUnitSphere(sampler), roughness);
		reflected = vecAdd(reflected, fuzz);
	}
	
	return (struct bsdfSample){.out = reflected, .color = metalBsdf->eval(record)};
}

struct metalBsdf *newMetal() {
	struct metalBsdf *new = calloc(1, sizeof(*new));
	new->eval = metal_color;
	new->bsdf.sample = metal_sample;
	return new;
}
