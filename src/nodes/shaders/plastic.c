//
//  plastic.c
//  C-Ray
//
//  Created by Valtteri Koskivuori on 01/12/2020.
//  Copyright © 2020 Valtteri Koskivuori. All rights reserved.
//

#include "../../includes.h"
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
	struct colorNode *color;
	struct colorNode *roughness;
};

// From diffuse.c
// FIXME: Find a better way to share sampling strategies between nodes.
struct bsdfSample sampleDiffuse(const struct bsdfNode *bsdf, sampler *sampler, const struct hitRecord *record);

struct bsdfSample sampleShiny(const struct bsdfNode *bsdf, sampler *sampler, const struct hitRecord *record) {
	struct plasticBsdf *plastic = (struct plasticBsdf *)bsdf;
	struct vector reflected = reflectVec(&record->incident.direction, &record->surfaceNormal);
	//Roughness
	float roughness = plastic->roughness->eval(plastic->roughness, record).red;
	if (roughness > 0.0f) {
		const struct vector fuzz = vecScale(randomOnUnitSphere(sampler), roughness);
		reflected = vecAdd(reflected, fuzz);
	}
	return (struct bsdfSample){
		.out = reflected,
		.color = whiteColor
	};
}

struct bsdfSample samplePlastic(const struct bsdfNode *bsdf, sampler *sampler, const struct hitRecord *record) {
	struct vector outwardNormal;
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
	
	if (refract(&record->incident.direction, outwardNormal, niOverNt, &refracted)) {
		reflectionProbability = schlick(cosine, record->material.IOR);
	} else {
		reflectionProbability = 1.0f;
	}
	
	if (getDimension(sampler) < reflectionProbability) {
		return sampleShiny(bsdf, sampler, record);
	} else {
		return sampleDiffuse(bsdf, sampler, record);
	}
}

static bool compare(const void *A, const void *B) {
	const struct plasticBsdf *this = A;
	const struct plasticBsdf *other = B;
	return this->color == other->color && this->roughness == other->roughness;
}

static uint32_t hash(const void *p) {
	const struct plasticBsdf *this = p;
	uint32_t h = hashInit();
	h = hashBytes(h, &this->color, sizeof(this->color));
	h = hashBytes(h, &this->roughness, sizeof(this->roughness));
	return h;
}

struct bsdfNode *newPlastic(struct world *world, struct colorNode *color) {
	HASH_CONS(world->nodeTable, &world->nodePool, hash, struct plasticBsdf, {
		.color = color ? color : newConstantTexture(world, blackColor),
		.roughness = newConstantTexture(world, blackColor),
		.bsdf = {
			.sample = samplePlastic,
			.base = { .compare = compare }
		}
	});
}
