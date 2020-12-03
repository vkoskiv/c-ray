//
//  glass.c
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

#include "glass.h"

struct glassBsdf {
	struct bsdf bsdf;
	struct textureNode *roughness;
	struct textureNode *color;
};

struct bsdfSample glass_sample(const struct bsdf *bsdf, sampler *sampler, const struct hitRecord *record, const struct vector *in) {
	struct glassBsdf *glassBsdf = (struct glassBsdf*)bsdf;
	
	struct vector outwardNormal;
	struct vector reflected = reflectVec(in, &record->surfaceNormal);
	float niOverNt;
	struct vector refracted;
	float reflectionProbability;
	float cosine;
	
	if (vecDot(*in, record->surfaceNormal) > 0.0f) {
		outwardNormal = vecNegate(record->surfaceNormal);
		niOverNt = record->material.IOR;
		cosine = record->material.IOR * vecDot(record->incident.direction, record->surfaceNormal) / vecLength(record->incident.direction);
	} else {
		outwardNormal = record->surfaceNormal;
		niOverNt = 1.0f / record->material.IOR;
		cosine = -(vecDot(*in, record->surfaceNormal) / vecLength(*in));
	}
	
	if (refract(in, outwardNormal, niOverNt, &refracted)) {
		reflectionProbability = schlick(cosine, record->material.IOR);
	} else {
		reflectionProbability = 1.0f;
	}
	
	float roughness = glassBsdf->roughness ? glassBsdf->roughness->eval(glassBsdf->roughness, record).red : record->material.roughness;
	if (roughness > 0.0f) {
		struct vector fuzz = vecScale(randomOnUnitSphere(sampler), roughness);
		reflected = vecAdd(reflected, fuzz);
		refracted = vecAdd(refracted, fuzz);
	}
	
	struct vector scatterDir = {0};
	if (getDimension(sampler) < reflectionProbability) {
		scatterDir = reflected;
	} else {
		scatterDir = refracted;
	}
	
	return (struct bsdfSample){.out = scatterDir, .color = glassBsdf->color->eval(glassBsdf->color, record)};
}

struct bsdf *newGlass(struct textureNode *color, struct textureNode *roughness) {
	struct glassBsdf *new = calloc(1, sizeof(*new));
	new->color = color;
	new->roughness = roughness;
	new->bsdf.sample = glass_sample;
	return (struct bsdf *)new;
}

