//
//  valuenode.c
//  C-Ray
//
//  Created by Valtteri Koskivuori on 16/12/2020.
//  Copyright © 2020 Valtteri Koskivuori. All rights reserved.
//

#include "../includes.h"
#include "../datatypes/color.h"
#include "../datatypes/vector.h"
#include "../datatypes/hitrecord.h"
#include "../datatypes/scene.h"
#include "../utils/hashtable.h"

#include "valuenode.h"

struct constantValue {
	struct valueNode node;
	float value;
};

static bool compare(const void *A, const void *B) {
	const struct constantValue *this = A;
	const struct constantValue *other = B;
	return this->value == other->value;
}

static uint32_t hash(const void *p) {
	const struct constantValue *this = p;
	uint32_t h = hashInit();
	h = hashBytes(h, &this->value, sizeof(this->value));
	return h;
}

static float eval(const struct valueNode *node, const struct hitRecord *record) {
	(void)record;
	struct constantValue *this = (struct constantValue *)node;
	return this->value;
}

const struct valueNode *newConstantValue(const struct world *world, float value) {
	HASH_CONS(world->nodeTable, world->nodePool, hash, struct constantValue, {
		.value = value,
		.node = {
			.eval = eval,
			.base = { .compare = compare }
		}
	});
}
