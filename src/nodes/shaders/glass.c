//
//  glass.c
//  C-Ray
//
//  Created by Valtteri Koskivuori on 30/11/2020.
//  Copyright © 2020 Valtteri Koskivuori. All rights reserved.
//

#include "../../includes.h"
#include "../../datatypes/color.h"
#include "../../renderer/samplers/sampler.h"
#include "../../datatypes/vector.h"
#include "../../datatypes/material.h"
#include "../../utils/hashtable.h"
#include "../../datatypes/scene.h"
#include "../bsdfnode.h"

#include "glass.h"

struct glassBsdf {
	struct bsdf bsdf;
	struct colorNode *color;
	struct colorNode *roughness;
	float IOR;
};

struct bsdfSample sampleGlass(const struct bsdf *bsdf, sampler *sampler, const struct hitRecord *record) {
	struct glassBsdf *glassBsdf = (struct glassBsdf*)bsdf;
	
	struct vector outwardNormal;
	struct vector reflected = reflectVec(&record->incident.direction, &record->surfaceNormal);
	float niOverNt;
	struct vector refracted;
	float reflectionProbability;
	float cosine;
	
	float IOR = glassBsdf->IOR;
	
	if (vecDot(record->incident.direction, record->surfaceNormal) > 0.0f) {
		outwardNormal = vecNegate(record->surfaceNormal);
		niOverNt = IOR;
		cosine = IOR * vecDot(record->incident.direction, record->surfaceNormal) / vecLength(record->incident.direction);
	} else {
		outwardNormal = record->surfaceNormal;
		niOverNt = 1.0f / IOR;
		cosine = -(vecDot(record->incident.direction, record->surfaceNormal) / vecLength(record->incident.direction));
	}
	
	if (refract(&record->incident.direction, outwardNormal, niOverNt, &refracted)) {
		reflectionProbability = schlick(cosine, IOR);
	} else {
		reflectionProbability = 1.0f;
	}
	
	float roughness = glassBsdf->roughness->eval(glassBsdf->roughness, record).red;
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

//TODO: Add IOR input
struct bsdf *newGlass(struct world *world, struct colorNode *color, struct colorNode *roughness, float IOR) {
	HASH_CONS(world->nodeTable, &world->nodePool, hash, struct glassBsdf, {
		.color = color ? color : newConstantTexture(world, blackColor),
		.roughness = roughness ? roughness : newConstantTexture(world, blackColor),
		.IOR = IOR,
		.bsdf = {
			.sample = sampleGlass,
			.base = { .compare = compare }
		}
	});
}

