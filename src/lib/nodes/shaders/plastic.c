//
//  plastic.c
//  c-ray
//
//  Created by Valtteri Koskivuori on 01/12/2020.
//  Copyright © 2020-2025 Valtteri Koskivuori. All rights reserved.
//

#include <stdio.h>
#include <renderer/samplers/sampler.h>
#include <renderer/samplers/vec.h>
#include <common/color.h>
#include <common/vector.h>
#include <common/hashtable.h>
#include <datatypes/hitrecord.h>
#include <datatypes/scene.h>
#include "../colornode.h"
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

static void dump(const void *node, char *dumpbuf, int bufsize) {
	struct plasticBsdf *self = (struct plasticBsdf *)node;
	char roughness[DUMPBUF_SIZE / 2] = "";
	char diffuse[DUMPBUF_SIZE / 2] = "";
	char clear_coat[DUMPBUF_SIZE / 2] = "";
	char IOR[DUMPBUF_SIZE / 2] = "";
	if (self->roughness->base.dump) self->roughness->base.dump(self->roughness, roughness, sizeof(roughness));
	if (self->diffuse->base.dump) self->diffuse->base.dump(self->diffuse, diffuse, sizeof(diffuse));
	if (self->clear_coat->base.dump) self->clear_coat->base.dump(self->clear_coat, clear_coat, sizeof(clear_coat));
	if (self->IOR->base.dump) self->IOR->base.dump(self->IOR, IOR, sizeof(IOR));
	snprintf(dumpbuf, bufsize, "plasticBsdf { roughness: %s, diffuse: %s, clear_coat: %s, IOR: %s }", roughness, diffuse, clear_coat, IOR);
}

static struct bsdfSample sampleShiny(const struct bsdfNode *bsdf, sampler *sampler, const struct hitRecord *record) {
	struct plasticBsdf *plastic = (struct plasticBsdf *)bsdf;
	struct vector reflected = vec_reflect(record->incident->direction, record->surfaceNormal);
	//Roughness
	float roughness = plastic->roughness->eval(plastic->roughness, sampler, record);
	if (roughness > 0.0f) {
		const struct vector fuzz = vec_scale(vec_on_unit_sphere(sampler), roughness);
		reflected = vec_add(reflected, fuzz);
	}
	return (struct bsdfSample){
		.out = { .start = record->hitPoint, .direction = reflected, .type = rt_reflection | (roughness == 0.0f ? rt_singular : rt_glossy) },
		.weight = plastic->clear_coat->eval(plastic->clear_coat, sampler, record)
	};
}

static struct bsdfSample sample(const struct bsdfNode *bsdf, sampler *sampler, const struct hitRecord *record) {
	struct vector outwardNormal;
	float niOverNt;
	struct vector refracted;
	float reflectionProbability;
	float cosine;
	
	struct plasticBsdf *this = (struct plasticBsdf *)bsdf;
	
	const float IOR = this->IOR->eval(this->IOR, sampler, record);
	
	if (vec_dot(record->incident->direction, record->surfaceNormal) > 0.0f) {
		outwardNormal = vec_negate(record->surfaceNormal);
		niOverNt = IOR;
		cosine = IOR * vec_dot(record->incident->direction, record->surfaceNormal) / vec_length(record->incident->direction);
	} else {
		outwardNormal = record->surfaceNormal;
		niOverNt = 1.0f / IOR;
		cosine = -(vec_dot(record->incident->direction, record->surfaceNormal) / vec_length(record->incident->direction));
	}
	
	if (vec_refract(record->incident->direction, outwardNormal, niOverNt, &refracted)) {
		reflectionProbability = schlick(cosine, IOR);
	} else {
		reflectionProbability = 1.0f;
	}
	
	if (sampler_dimension(sampler) < reflectionProbability) {
		return sampleShiny(bsdf, sampler, record);
	} else {
		return this->diffuse->sample(this->diffuse, sampler, record);
	}
}

// TODO: Separate clear coat + base colors
const struct bsdfNode *newPlastic(const struct node_storage *s, const struct colorNode *color, const struct valueNode *roughness, const struct valueNode *IOR) {
	HASH_CONS(s->node_table, hash, struct plasticBsdf, {
		.diffuse = newDiffuse(s, color),
		.clear_coat = color ? color : newConstantTexture(s, g_white_color),
		.roughness = roughness ? roughness : newConstantValue(s, 0.0f),
		.IOR = IOR ? IOR : newConstantValue(s, 1.45f),
		.bsdf = {
			.sample = sample,
			.base = { .compare = compare, .dump = dump }
		}
	});
}
