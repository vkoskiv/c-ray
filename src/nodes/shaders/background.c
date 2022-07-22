//
//  background.c
//  C-Ray
//
//  Created by Valtteri Koskivuori on 19/12/2020.
//  Copyright © 2020-2022 Valtteri Koskivuori. All rights reserved.
//

#include "../../datatypes/color.h"
#include "../../datatypes/vector.h"
#include "../../datatypes/hitrecord.h"
#include "../../utils/hashtable.h"
#include "../../datatypes/scene.h"
#include "../bsdfnode.h"

#include "background.h"

struct backgroundBsdf {
	struct bsdfNode bsdf;
	const struct colorNode *color;
	const struct valueNode *strength;
};

static bool compare(const void *A, const void *B) {
	const struct backgroundBsdf *this = A;
	const struct backgroundBsdf *other = B;
	return this->color == other->color;
}

static uint32_t hash(const void *p) {
	const struct backgroundBsdf *this = p;
	uint32_t h = hashInit();
	h = hashBytes(h, &this->color, sizeof(this->color));
	return h;
}

static struct bsdfSample sample(const struct bsdfNode *bsdf, sampler *sampler, const struct hitRecord *record) {
	(void)sampler;
	struct backgroundBsdf *background = (struct backgroundBsdf *)bsdf;
	float strength = background->strength->eval(background->strength, record);
	return (struct bsdfSample){
		.out = vecZero(),
		.color = colorCoef(strength, background->color->eval(background->color, record))
	};
}

const struct bsdfNode *newBackground(const struct node_storage *s, const struct colorNode *tex, const struct valueNode *strength) {
	HASH_CONS(s->node_table, hash, struct backgroundBsdf, {
		.color = tex ? tex : newConstantTexture(s, grayColor),
		.strength = strength ? strength : newConstantValue(s, 1.0f),
		.bsdf = {
			.sample = sample,
			.base = { .compare = compare }
		}
	});
}
