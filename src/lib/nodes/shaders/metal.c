//
//  metal.c
//  C-Ray
//
//  Created by Valtteri Koskivuori on 30/11/2020.
//  Copyright © 2020-2022 Valtteri Koskivuori. All rights reserved.
//

#include <stdio.h>
#include "../../../common/color.h"
#include "../../renderer/samplers/sampler.h"
#include "../../renderer/samplers/vec.h"
#include "../../../common/vector.h"
#include "../../../common/hashtable.h"
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

static void dump(const void *node, char *dumpbuf, int bufsize) {
	struct metalBsdf *self = (struct metalBsdf *)node;
	char color[DUMPBUF_SIZE / 2] = "";
	char roughness[DUMPBUF_SIZE / 2] = "";
	if (self->color->base.dump) self->color->base.dump(self->color, color, sizeof(color));
	if (self->roughness->base.dump) self->roughness->base.dump(self->roughness, roughness, sizeof(roughness));
	snprintf(dumpbuf, bufsize, "metalBsdf { color: %s, roughness: %s }", color, roughness);
}

static struct bsdfSample sample(const struct bsdfNode *bsdf, sampler *sampler, const struct hitRecord *record) {
	struct metalBsdf *metalBsdf = (struct metalBsdf *)bsdf;
	
	const struct vector normalizedDir = vec_normalize(record->incident_dir);
	struct vector reflected = vec_reflect(normalizedDir, record->surfaceNormal);
	float roughness = metalBsdf->roughness->eval(metalBsdf->roughness, sampler, record);
	if (roughness > 0.0f) {
		const struct vector fuzz = vec_scale(vec_on_unit_sphere(sampler), roughness);
		reflected = vec_add(reflected, fuzz);
	}
	
	return (struct bsdfSample){
		.out = reflected,
		.weight = metalBsdf->color->eval(metalBsdf->color, sampler, record)
	};
}

const struct bsdfNode *newMetal(const struct node_storage *s, const struct colorNode *color, const struct valueNode *roughness) {
	HASH_CONS(s->node_table, hash, struct metalBsdf, {
		.color = color ? color : newConstantTexture(s, g_black_color),
		.roughness = roughness ? roughness : newConstantValue(s, 0.0f),
		.bsdf = {
			.sample = sample,
			.base = { .compare = compare, .dump = dump }
		}
	});
}
