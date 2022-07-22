//
//  metal.c
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

#include "metal.h"

struct metalBsdf {
	struct bsdfNode bsdf;
	const struct colorNode *color;
	const struct valueNode *roughness;
};

static bool compare(const void *A, const void *B) {
	const struct metalBsdf *this = A;
	const struct metalBsdf *other = B;
	return this->color == other->color && this->roughness == other->roughness;
}

static uint32_t hash(const void *p) {
	const struct metalBsdf *this = p;
	uint32_t h = hashInit();
	h = hashBytes(h, &this->color, sizeof(this->color));
	h = hashBytes(h, &this->roughness, sizeof(this->roughness));
	return h;
}

static struct bsdfSample sample(const struct bsdfNode *bsdf, sampler *sampler, const struct hitRecord *record) {
	struct metalBsdf *metalBsdf = (struct metalBsdf *)bsdf;
	
	const struct vector normalizedDir = vecNormalize(record->incident_dir);
	struct vector reflected = vecReflect(normalizedDir, record->surfaceNormal);
	float roughness = metalBsdf->roughness->eval(metalBsdf->roughness, record);
	if (roughness > 0.0f) {
		const struct vector fuzz = vecScale(randomOnUnitSphere(sampler), roughness);
		reflected = vecAdd(reflected, fuzz);
	}
	
	return (struct bsdfSample){
		.out = reflected,
		.color = metalBsdf->color->eval(metalBsdf->color, record)
	};
}

const struct bsdfNode *newMetal(const struct node_storage *s, const struct colorNode *color, const struct valueNode *roughness) {
	HASH_CONS(s->node_table, hash, struct metalBsdf, {
		.color = color ? color : newConstantTexture(s, blackColor),
		.roughness = roughness ? roughness : newConstantValue(s, 0.0f),
		.bsdf = {
			.sample = sample,
			.base = { .compare = compare }
		}
	});
}
