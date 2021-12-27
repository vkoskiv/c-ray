//
//  blackbody.c
//  C-Ray
//
//  Created by Valtteri Koskivuori on 16/12/2020.
//  Copyright © 2020-2021 Valtteri Koskivuori. All rights reserved.
//

#include "../../includes.h"
#include "../nodebase.h"

#include "../../utils/hashtable.h"
#include "../../datatypes/scene.h"
#include "../../datatypes/hitrecord.h"
#include "../colornode.h"
#include "../valuenode.h"

#include "blackbody.h"

struct blackbodyNode {
	struct colorNode node;
	const struct valueNode *temperature;
};

static bool compare(const void *A, const void *B) {
	const struct blackbodyNode *this = A;
	const struct blackbodyNode *other = B;
	return this->temperature == other->temperature;
}

static uint32_t hash(const void *p) {
	const struct blackbodyNode *this = p;
	uint32_t h = hashInit();
	h = hashBytes(h, &this->temperature, sizeof(this->temperature));
	return h;
}

static struct color eval(const struct colorNode *node, const struct hitRecord *record) {
	(void)record;
	struct blackbodyNode *this = (struct blackbodyNode *)node;
	return colorForKelvin(this->temperature->eval(this->temperature, record));
}

const struct colorNode *newBlackbody(const struct world *world, const struct valueNode *temperature) {
	HASH_CONS(world->nodeTable, hash, struct blackbodyNode, {
		.temperature = temperature ? temperature : newConstantValue(world, 4000.0f),
		.node = {
			.eval = eval,
			.base = { .compare = compare }
		}
	});
}
