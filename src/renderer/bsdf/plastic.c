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
	struct textureNode *roughness;
};

// From diffuse.c
// FIXME: Find a better way to share sampling strategies between nodes.
struct bsdfSample sampleDiffuse(const struct bsdf *bsdf, sampler *sampler, const struct hitRecord *record, const struct vector *in);

struct bsdfSample sampleShiny(const struct bsdf *bsdf, sampler *sampler, const struct hitRecord *record, const struct vector *in) {
	struct plasticBsdf *plastic = (struct plasticBsdf *)bsdf;
	struct vector reflected = reflectVec(in, &record->surfaceNormal);
	//Roughness
	float roughness = plastic->roughness ? plastic->roughness->eval(plastic->roughness, record).red : record->material.roughness;
	if (roughness > 0.0f) {
		const struct vector fuzz = vecScale(randomOnUnitSphere(sampler), roughness);
		reflected = vecAdd(reflected, fuzz);
	}
	return (struct bsdfSample){.out = reflected, .color = whiteColor};
}

struct bsdfSample plastic_sample(const struct bsdf *bsdf, sampler *sampler, const struct hitRecord *record, const struct vector *in) {
	(void)in;
	struct vector outwardNormal;
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
	
	if (refract(&record->incident.direction, outwardNormal, niOverNt, &refracted)) {
		reflectionProbability = schlick(cosine, record->material.IOR);
	} else {
		reflectionProbability = 1.0f;
	}
	
	if (getDimension(sampler) < reflectionProbability) {
		return sampleShiny(bsdf, sampler, record, in);
	} else {
		return sampleDiffuse(bsdf, sampler, record, in);
	}
}

struct bsdf *newPlastic(struct block **pool, struct textureNode *tex) {
	struct plasticBsdf *new = allocBlock(pool, sizeof(*new));
	new->color = tex;
	new->bsdf.sample = plastic_sample;
	return (struct bsdf *)new;
}
