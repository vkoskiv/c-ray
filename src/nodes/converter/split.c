//
//  split.c
//  C-Ray
//
//  Created by Valtteri Koskivuori on 17/12/2020.
//  Copyright © 2020-2021 Valtteri Koskivuori. All rights reserved.
//

#include "../../includes.h"
#include "../../datatypes/color.h"
#include "../../utils/assert.h"
#include "../../utils/mempool.h"
#include "../../datatypes/hitrecord.h"
#include "../../utils/hashtable.h"
#include "../../datatypes/scene.h"
#include "../valuenode.h"

#include "split.h"

struct splitValue {
	struct colorNode node;
	const struct valueNode *input;
};

static bool compare(const void *A, const void *B) {
	const struct splitValue *this = A;
	const struct splitValue *other = B;
	return this->input == other->input;
}

static uint32_t hash(const void *p) {
	const struct splitValue *this = p;
	uint32_t h = hashInit();
	h = hashBytes(h, &this->input, sizeof(this->input));
	return h;
}

static struct color eval(const struct colorNode *node, const struct hitRecord *record) {
	const struct splitValue *this = (struct splitValue *)node;
	float val = this->input->eval(this->input, record);
	//TODO: What do we do with the alpha here?
	return (struct color){val, val, val, 1.0f};
}

const struct colorNode *newSplitValue(const struct world *world, const struct valueNode *node) {
	HASH_CONS(world->nodeTable, hash, struct splitValue, {
		.input = node ? node : newConstantValue(world, 0.0f),
		.node = {
			.eval = eval,
			.base = { .compare = compare }
		}
	});
}

