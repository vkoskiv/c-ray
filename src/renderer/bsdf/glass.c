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

static inline bool refract(struct vector in, struct vector normal, float niOverNt, struct vector *refracted) {
	const struct vector uv = vecNormalize(in);
	const float dt = vecDot(uv, normal);
	const float discriminant = 1.0f - niOverNt * niOverNt * (1.0f - dt * dt);
	if (discriminant > 0.0f) {
		const struct vector A = vecScale(normal, dt);
		const struct vector B = vecSub(uv, A);
		const struct vector C = vecScale(B, niOverNt);
		const struct vector D = vecScale(normal, sqrtf(discriminant));
		*refracted = vecSub(C, D);
		return true;
	} else {
		return false;
	}
}

static inline float schlick(float cosine, float IOR) {
	float r0 = (1.0f - IOR) / (1.0f + IOR);
	r0 = r0 * r0;
	return r0 + (1.0f - r0) * powf((1.0f - cosine), 5.0f);
}

struct color glass_color(const struct hitRecord *record) {
	return record->material.texture ? colorForUV(record, Diffuse) : record->material.diffuse;
}

struct bsdfSample glass_sample(const struct bsdf *bsdf, sampler *sampler, const struct hitRecord *record, const struct vector *in) {
	(void)in;
	struct glassBsdf *glassBsdf = (struct glassBsdf*)bsdf;
	
	struct vector outwardNormal;
	struct vector reflected = reflectVec(&record->incident.direction, &record->surfaceNormal);
	float niOverNt;
	struct vector refracted;
	float reflectionProbability;
	float cosine;
	
	if (vecDot(record->incident.direction, record->surfaceNormal) > 0.0f) {
		outwardNormal = vecNegate(record->surfaceNormal);
		niOverNt = record->material.IOR;
		cosine = record->material.IOR * vecDot(record->incident.direction, record->surfaceNormal) / vecLength(record->incident.direction);
	} else {
		outwardNormal = record->surfaceNormal;
		niOverNt = 1.0f / record->material.IOR;
		cosine = -(vecDot(record->incident.direction, record->surfaceNormal) / vecLength(record->incident.direction));
	}
	
	if (refract(record->incident.direction, outwardNormal, niOverNt, &refracted)) {
		reflectionProbability = schlick(cosine, record->material.IOR);
	} else {
		reflectionProbability = 1.0f;
	}
	
	//Roughness
	float roughness = record->material.specularMap ? colorForUV(record, Specular).red : record->material.roughness;
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
	
	return (struct bsdfSample){.out = scatterDir, .color = glassBsdf->eval(record)};
}

struct glassBsdf *newGlass() {
	struct glassBsdf *new = calloc(1, sizeof(*new));
	new->eval = glass_color;
	new->bsdf.sample = glass_sample;
	return new;
}

