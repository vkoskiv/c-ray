//
//  combine.c
//  C-Ray
//
//  Created by Valtteri Koskivuori on 17/12/2020.
//  Copyright © 2020 Valtteri Koskivuori. All rights reserved.
//

#include "../../includes.h"
#include "../../datatypes/color.h"
#include "../../utils/assert.h"
#include "../../utils/mempool.h"
#include "../../datatypes/hitrecord.h"
#include "../../utils/hashtable.h"
#include "../../datatypes/scene.h"
#include "../valuenode.h"

#include "combine.h"

struct combineValue {
	struct colorNode node;
	const struct valueNode *original;
};

static bool compare(const void *A, const void *B) {
	const struct combineValue *this = A;
	const struct combineValue *other = B;
	return this->original == other->original;
}

static uint32_t hash(const void *p) {
	const struct combineValue *this = p;
	uint32_t h = hashInit();
	h = hashBytes(h, &this->original, sizeof(this->original));
	return h;
}

static struct color eval(const struct colorNode *node, const struct hitRecord *record) {
	const struct combineValue *this = (struct combineValue *)node;
	float val = this->original->eval(this->original, record);
	//TODO: What do we do with the alpha here?
	return (struct color){val, val, val, 1.0f};
}

const struct colorNode *newCombineValue(const struct world *world, const struct valueNode *node) {
	HASH_CONS(world->nodeTable, world->nodePool, hash, struct combineValue, {
		.original = node ? node : newConstantValue(world, 0.0f),
		.node = {
			.eval = eval,
			.base = { .compare = compare }
		}
	});
}

