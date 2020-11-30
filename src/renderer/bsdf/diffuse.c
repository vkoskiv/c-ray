//
//  diffuse.c
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

#include "diffuse.h"


/*bool lambertianBSDF(const struct hitRecord *isect, struct color *attenuation, struct lightRay *scattered, sampler *sampler) {
	const struct vector scatterDir = vecNormalize(vecAdd(isect->surfaceNormal, randomOnUnitSphere(sampler)));
	*scattered = ((struct lightRay){isect->hitPoint, scatterDir, rayTypeScattered});
	*attenuation = diffuseColor(isect);
	return true;
}*/

struct color bsdf_color(const struct hitRecord *record) {
	return record->material.texture ? colorForUV(record, Diffuse) : record->material.diffuse;
}

struct bsdfSample bsdf_sample(const struct bsdf *bsdf, sampler *sampler, const struct hitRecord *record, const struct vector *in) {
	(void)in;
	struct diffuseBsdf *diffBsdf = (struct diffuseBsdf*)bsdf;
	const struct vector scatterDir = vecNormalize(vecAdd(record->surfaceNormal, randomOnUnitSphere(sampler)));
	return (struct bsdfSample){.out = scatterDir, .color = diffBsdf->eval(record)};
}

struct diffuseBsdf *newDiffuse() {
	struct diffuseBsdf *new = calloc(1, sizeof(*new));
	new->eval = bsdf_color;
	new->bsdf.sample = bsdf_sample;
	return new;
}
