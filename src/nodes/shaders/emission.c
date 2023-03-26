//
//  emission.c
//  C-ray
//
//  Created by Valtteri on 2.1.2021.
//  Copyright © 2021-2022 Valtteri Koskivuori. All rights reserved.
//

#include <stdio.h>
#include "../../datatypes/color.h"
#include "../../renderer/samplers/sampler.h"
#include "../../datatypes/vector.h"
#include "../../datatypes/material.h"
#include "../colornode.h"
#include "../../utils/hashtable.h"
#include "../../datatypes/scene.h"
#include "../bsdfnode.h"

#include "emission.h"

struct emissiveBsdf {
	struct bsdfNode bsdf;
	const struct colorNode *color;
	const struct valueNode *strength;
};

static bool compare(const void *A, const void *B) {
	const struct emissiveBsdf *this = A;
	const struct emissiveBsdf *other = B;
	return this->color == other->color && this->strength == other->strength;
}

static uint32_t hash(const void *p) {
	const struct emissiveBsdf *this = p;
	uint32_t h = hashInit();
	h = hashBytes(h, &this->color, sizeof(this->color));
	h = hashBytes(h, &this->strength, sizeof(this->strength));
	return h;
}

static void dump(const void *node, char *dumpbuf, int bufsize) {
	struct emissiveBsdf *self = (struct emissiveBsdf *)node;
	char color[DUMPBUF_SIZE / 2] = "";
	char strength[DUMPBUF_SIZE / 2] = "";
	if (self->color->base.dump) self->color->base.dump(self->color, color, sizeof(color));
	if (self->strength->base.dump) self->strength->base.dump(self->strength, strength, sizeof(strength));
	snprintf(dumpbuf, bufsize, "emissiveBsdf { color: %s, strength: %s }", color, strength);
}

static struct bsdfSample sample(const struct bsdfNode *bsdf, sampler *sampler, const struct hitRecord *record) {
	struct emissiveBsdf *emitBsdf = (struct emissiveBsdf *)bsdf;
	const struct vector scatterDir = vec_normalize(vec_add(record->surfaceNormal, vec_on_unit_sphere(sampler)));
	return (struct bsdfSample){
		.out = scatterDir,
		.color = colorCoef(emitBsdf->strength->eval(emitBsdf->strength, sampler, record), emitBsdf->color->eval(emitBsdf->color, sampler, record))
	};
}

const struct bsdfNode *newEmission(const struct node_storage *s, const struct colorNode *color, const struct valueNode *strength) {
	HASH_CONS(s->node_table, hash, struct emissiveBsdf, {
		.color = color ? color : newConstantTexture(s, g_black_color),
		.strength = strength ? strength : newConstantValue(s, 1.0f),
		.bsdf = {
			.sample = sample,
			.base = { .compare = compare, .dump = dump }
		}
	});
}
