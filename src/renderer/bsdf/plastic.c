//
//  plastic.c
//  C-Ray
//
//  Created by Valtteri Koskivuori on 01/12/2020.
//  Copyright © 2020 Valtteri Koskivuori. All rights reserved.
//

#include "../../includes.h"
#include "../../datatypes/color.h"
#include "../samplers/sampler.h"
#include "../../datatypes/vector.h"
#include "../../datatypes/material.h"
#include "texturenode.h"
#include "bsdf.h"

#include "plastic.h"

struct plasticBsdf {
	struct bsdf bsdf;
	struct textureNode *color;
};

/*struct bsdfSample diffuse_sample(const struct bsdf *bsdf, sampler *sampler, const struct hitRecord *record, const struct vector *in) {
	(void)in;
	struct diffuseBsdf *diffBsdf = (struct diffuseBsdf*)bsdf;
	const struct vector scatterDir = vecNormalize(vecAdd(record->surfaceNormal, randomOnUnitSphere(sampler)));
	return (struct bsdfSample){.out = scatterDir, .color = diffBsdf->color->eval(diffBsdf->color, record)};
}

struct bsdfSample shiny_sample(const struct bsdf *bsdf, sampler *sampler, const struct hitRecord *record, const struct vector *in) {
	struct vector reflected = reflectVec(in, &record->surfaceNormal);
	//Roughness
	float roughness = roughnessValue(isect);
	if (roughness > 0.0f) {
		const struct vector fuzz = vecScale(randomOnUnitSphere(sampler), roughness);
		reflected = vecAdd(reflected, fuzz);
	}
	*scattered = newRay(isect->hitPoint, reflected, rayTypeReflected);
	return true;
}

// Glossy plastic
struct bsdfSample plastic_sample(const struct bsdf *bsdf, sampler *sampler, const struct hitRecord *record, const struct vector *in) {
	struct vector outwardNormal;
	struct vector reflected = reflectVec(in, &record->surfaceNormal);
	float niOverNt;
	struct vector refracted;
	float reflectionProbability;
	float cosine;
	
	if (vecDot(*in, record->surfaceNormal) > 0.0f) {
		outwardNormal = vecNegate(record->surfaceNormal);
		niOverNt = record->material.IOR;
		cosine = record->material.IOR * vecDot(*in, record->surfaceNormal) / vecLength(*in);
	} else {
		outwardNormal = record->surfaceNormal;
		niOverNt = 1.0f / record->material.IOR;
		cosine = -(vecDot(*in, record->surfaceNormal) / vecLength(*in));
	}
	
	if (refract(isect->incident.direction, outwardNormal, niOverNt, &refracted)) {
		reflectionProbability = schlick(cosine, isect->material.IOR);
	} else {
		*scattered = newRay(isect->hitPoint, reflected, rayTypeReflected);
		reflectionProbability = 1.0f;
	}
	
	if (getDimension(sampler) < reflectionProbability) {
		return shiny_sample(bsdf, sampler, record, in);
	} else {
		return diffuse_sample(bsdf, sampler, record, in);
	}
}

struct bsdfSample plastic_sample(const struct bsdf *bsdf, sampler *sampler, const struct hitRecord *record, const struct vector *in) {
	(void)in;
	struct plasticBsdf *plasBsdf = (struct plasticBsdf*)bsdf;
	const struct vector scatterDir = vecNormalize(vecAdd(record->surfaceNormal, randomOnUnitSphere(sampler)));
	return (struct bsdfSample){.out = scatterDir, .color = plasBsdf->color->eval(plasBsdf->color, record)};
}

struct plasticBsdf *newPlastic(struct textureNode *tex) {
	struct plasticBsdf *new = calloc(1, sizeof(*new));
	new->color = tex;
	new->bsdf.sample = plastic_sample;
	return new;
}
*/
