//
//  isotropic.c
//  c-ray
//
//  Created by Valtteri on 27.5.2021.
//  Copyright © 2021-2025 Valtteri Koskivuori. All rights reserved.
//

#include <stdio.h>
#include <common/color.h>
#include <common/vector.h>
#include <common/hashtable.h>
#include <datatypes/hitrecord.h>
#include <datatypes/scene.h>
#include <renderer/samplers/sampler.h>
#include <renderer/samplers/vec.h>
#include "../colornode.h"
#include "../bsdfnode.h"

#include "isotropic.h"

struct isotropicBsdf {
	struct bsdfNode bsdf;
	const struct colorNode *color;
};

static bool compare(const void *A, const void *B) {
	const struct isotropicBsdf *this = A;
	const struct isotropicBsdf *other = B;
	return this->color == other->color;
}

static uint32_t hash(const void *p) {
	const struct isotropicBsdf *this = p;
	uint32_t h = hashInit();
	h = hashBytes(h, &this->color, sizeof(this->color));
	return h;
}

static void dump(const void *node, char *dumpbuf, int bufsize) {
	struct isotropicBsdf *self = (struct isotropicBsdf *)node;
	char color[DUMPBUF_SIZE / 2] = "";
	if (self->color->base.dump) self->color->base.dump(self->color, color, sizeof(color));
	snprintf(dumpbuf, bufsize, "isotropicBsdf { color: %s }", color);
}

static struct bsdfSample sample(const struct bsdfNode *bsdf, sampler *sampler, const struct hitRecord *record) {
	struct isotropicBsdf *isoBsdf = (struct isotropicBsdf *)bsdf;
	const struct vector scatterDir = vec_on_unit_sphere(sampler);
	return (struct bsdfSample){
		.out = { .start= record->hitPoint, .direction = scatterDir, .type = rt_transmission | rt_diffuse },
		.weight = isoBsdf->color->eval(isoBsdf->color, sampler, record)
	};
}

const struct bsdfNode *newIsotropic(const struct node_storage *s, const struct colorNode *color) {
	HASH_CONS(s->node_table, hash, struct isotropicBsdf, {
		.color = color ? color : newConstantTexture(s, g_black_color),
		.bsdf = {
			.sample = sample,
			.base = { .compare = compare, .dump = dump }
		}
	});
}
