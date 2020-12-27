//
//  transparent.c
//  C-Ray
//
//  Created by Valtteri Koskivuori on 16/12/2020.
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
#include "../../utils/logging.h"
#include "../bsdfnode.h"

#include "transparent.h"

struct transparent {
	struct bsdfNode bsdf;
	const struct colorNode *color;
};

static bool compare(const void *A, const void *B) {
	const struct transparent *this = A;
	const struct transparent *other = B;
	return this->color == other->color;
}

static uint32_t hash(const void *p) {
	const struct transparent *thing = p;
	uint32_t h = hashInit();
	h = hashBytes(h, &thing->color, sizeof(thing->color));
	return h;
}

static struct bsdfSample sample(const struct bsdfNode *bsdf, sampler *sampler, const struct hitRecord *record) {
	(void)sampler;
	struct transparent *this = (struct transparent *)bsdf;
	return (struct bsdfSample){ .out = record->incident.direction, .color = this->color->eval(this->color, record) };
}

const struct bsdfNode *newTransparent(const struct world *world, const struct colorNode *color) {
	HASH_CONS(world->nodeTable, hash, struct transparent, {
		.color = color ? color : newConstantTexture(world, whiteColor),
		.bsdf = {
			.sample = sample,
			.base = { .compare = compare }
		}
	});
}
