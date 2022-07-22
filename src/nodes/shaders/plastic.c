//
//  plastic.c
//  C-Ray
//
//  Created by Valtteri Koskivuori on 01/12/2020.
//  Copyright © 2020-2022 Valtteri Koskivuori. All rights reserved.
//

#include "../../datatypes/color.h"
#include "../../renderer/samplers/sampler.h"
#include "../../datatypes/vector.h"
#include "../../datatypes/material.h"
#include "../colornode.h"
#include "../../utils/hashtable.h"
#include "../../datatypes/scene.h"
#include "../bsdfnode.h"

#include "plastic.h"

struct plasticBsdf {
	struct bsdfNode bsdf;
	const struct valueNode *roughness;
	const struct bsdfNode *diffuse;
	const struct colorNode *clear_coat;
	const struct valueNode *IOR;
};

static bool compare(const void *A, const void *B) {
	const struct plasticBsdf *this = A;
	const struct plasticBsdf *other = B;
	return this->roughness == other->roughness && this->diffuse == other->diffuse && this->clear_coat == other->clear_coat && this->IOR == other->IOR;
}

static uint32_t hash(const void *p) {
	const struct plasticBsdf *this = p;
	uint32_t h = hashInit();
	h = hashBytes(h, &this->roughness, sizeof(this->roughness));
	h = hashBytes(h, &this->diffuse, sizeof(this->diffuse));
	h = hashBytes(h, &this->clear_coat, sizeof(this->clear_coat));
	h = hashBytes(h, &this->IOR, sizeof(this->IOR));
	return h;
}

static struct bsdfSample sampleShiny(const struct bsdfNode *bsdf, sampler *sampler, const struct hitRecord *record) {
	struct plasticBsdf *plastic = (struct plasticBsdf *)bsdf;
	struct vector reflected = vecReflect(record->incident_dir, record->surfaceNormal);
	//Roughness
	float roughness = plastic->roughness->eval(plastic->roughness, record);
	if (roughness > 0.0f) {
		const struct vector fuzz = vecScale(randomOnUnitSphere(sampler), roughness);
		reflected = vecAdd(reflected, fuzz);
	}
	return (struct bsdfSample){
		.out = reflected,
		.color = plastic->clear_coat->eval(plastic->clear_coat, record)
	};
}

static struct bsdfSample sample(const struct bsdfNode *bsdf, sampler *sampler, const struct hitRecord *record) {
	struct vector outwardNormal;
	float niOverNt;
	struct vector refracted;
	float reflectionProbability;
	float cosine;
	
	struct plasticBsdf *this = (struct plasticBsdf *)bsdf;
	
	const float IOR = this->IOR->eval(this->IOR, record);
	
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
	
	if (getDimension(sampler) < reflectionProbability) {
		return sampleShiny(bsdf, sampler, record);
	} else {
		return this->diffuse->sample(this->diffuse, sampler, record);
	}
}

const struct bsdfNode *newPlastic(const struct node_storage *s, const struct colorNode *color, const struct valueNode *roughness, const struct valueNode *IOR) {
	HASH_CONS(s->node_table, hash, struct plasticBsdf, {
		.diffuse = newDiffuse(s, color),
		.clear_coat = color ? color : newConstantTexture(s, whiteColor),
		.roughness = roughness ? roughness : newConstantValue(s, 0.0f),
		.IOR = IOR ? IOR : newConstantValue(s, 1.45f),
		.bsdf = {
			.sample = sample,
			.base = { .compare = compare }
		}
	});
}
