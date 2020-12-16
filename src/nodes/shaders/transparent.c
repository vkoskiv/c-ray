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
#include "../textures/texturenode.h"
#include "../../utils/hashtable.h"
#include "../../datatypes/scene.h"
#include "../../utils/logging.h"
#include "bsdf.h"

#include "transparent.h"

struct transparent {
	struct bsdf bsdf;
	struct colorNode *color;
};

static struct bsdfSample sample(const struct bsdf *bsdf, sampler *sampler, const struct hitRecord *record) {
	(void)sampler;
	struct transparent *this = (struct transparent *)bsdf;
	return (struct bsdfSample){ .out = record->incident.direction, .color = this->color->eval(this->color, record) };
}

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

struct bsdf *newTransparent(struct world *world, struct colorNode *color) {
	HASH_CONS(world->nodeTable, &world->nodePool, hash, struct transparent, {
		.color = color ? color : newConstantTexture(world, whiteColor),
		.bsdf = {
			.sample = sample,
			.base = { .compare = compare }
		}
	});
}
