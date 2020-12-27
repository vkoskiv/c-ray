//
//  grayscale.c
//  C-Ray
//
//  Created by Valtteri Koskivuori on 15/12/2020.
//  Copyright © 2020 Valtteri Koskivuori. All rights reserved.
//

#include "../../includes.h"
#include "../../datatypes/color.h"
#include "../../datatypes/poly.h"
#include "../../utils/assert.h"
#include "../../utils/mempool.h"
#include "../../datatypes/hitrecord.h"
#include "../../utils/hashtable.h"
#include "../../datatypes/scene.h"
#include "../valuenode.h"
#include "../colornode.h"

#include "grayscale.h"

struct grayscale {
	struct valueNode node;
	const struct colorNode *original;
};

static bool compare(const void *A, const void *B) {
	const struct grayscale *this = A;
	const struct grayscale *other = B;
	return this->original == other->original;
}

static uint32_t hash(const void *p) {
	const struct grayscale *this = p;
	uint32_t h = hashInit();
	h = hashBytes(h, &this->original, sizeof(this->original));
	return h;
}

static float eval(const struct valueNode *node, const struct hitRecord *record) {
	const struct grayscale *this = (struct grayscale *)node;
	return grayscale(this->original->eval(this->original, record)).red;
}

const struct valueNode *newGrayscaleConverter(const struct world *world, const struct colorNode *node) {
	HASH_CONS(world->nodeTable, world->nodePool, hash, struct grayscale, {
		.original = node ? node : newConstantTexture(world, blackColor),
		.node = {
			.eval = eval,
			.base = { .compare = compare }
		}
	});
}
