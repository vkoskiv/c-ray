//
//  glass.c
//  C-Ray
//
//  Created by Valtteri Koskivuori on 30/11/2020.
//  Copyright © 2020-2022 Valtteri Koskivuori. All rights reserved.
//

#include "../../datatypes/color.h"
#include "../../renderer/samplers/sampler.h"
#include "../../datatypes/vector.h"
#include "../../datatypes/material.h"
#include "../../utils/hashtable.h"
#include "../../datatypes/scene.h"
#include "../bsdfnode.h"

#include "glass.h"

struct glassBsdf {
	struct bsdfNode bsdf;
	const struct colorNode *color;
	const struct valueNode *roughness;
	const struct valueNode *IOR;
};

static bool compare(const void *A, const void *B) {
	const struct glassBsdf *this = A;
	const struct glassBsdf *other = B;
	return this->color == other->color && this->roughness == other->roughness;
}

static uint32_t hash(const void *p) {
	const struct glassBsdf *this = p;
	uint32_t h = hashInit();
	h = hashBytes(h, &this->color, sizeof(this->color));
	h = hashBytes(h, &this->roughness, sizeof(this->roughness));
	return h;
}

static struct bsdfSample sample(const struct bsdfNode *bsdf, sampler *sampler, const struct hitRecord *record) {
	struct glassBsdf *glassBsdf = (struct glassBsdf *)bsdf;
	
	struct vector outwardNormal;
	struct vector reflected = vecReflect(record->incident_dir, record->surfaceNormal);
	float niOverNt;
	struct vector refracted;
	float reflectionProbability;
	float cosine;
	
	float IOR = glassBsdf->IOR->eval(glassBsdf->IOR, record);
	
	if (vecDot(record->incident_dir, record->surfaceNormal) > 0.0f) {
		outwardNormal = vecNegate(record->surfaceNormal);
		niOverNt = IOR;
		cosine = IOR * vecDot(record->incident_dir, record->surfaceNormal) / vecLength(record->incident_dir);
	} else {
		outwardNormal = record->surfaceNormal;
		niOverNt = 1.0f / IOR;
		cosine = -(vecDot(record->incident_dir, record->surfaceNormal) / vecLength(record->incident_dir));
	}
	
	if (refract(&record->incident_dir, outwardNormal, niOverNt, &refracted)) {
		reflectionProbability = schlick(cosine, IOR);
	} else {
		reflectionProbability = 1.0f;
	}
	
	float roughness = glassBsdf->roughness->eval(glassBsdf->roughness, record);
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
	
	return (struct bsdfSample){
		.out = scatterDir,
		.color = glassBsdf->color->eval(glassBsdf->color, record)
	};
}

const struct bsdfNode *newGlass(const struct node_storage *s, const struct colorNode *color, const struct valueNode *roughness, const struct valueNode *IOR) {
	HASH_CONS(s->node_table, hash, struct glassBsdf, {
		.color = color ? color : newConstantTexture(s, blackColor),
		.roughness = roughness ? roughness : newConstantValue(s, 0.0f),
		.IOR = IOR ? IOR : newConstantValue(s, 1.45f),
		.bsdf = {
			.sample = sample,
			.base = { .compare = compare }
		}
	});
}

