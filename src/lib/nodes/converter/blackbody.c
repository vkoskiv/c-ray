//
//  blackbody.c
//  C-Ray
//
//  Created by Valtteri Koskivuori on 16/12/2020.
//  Copyright © 2020-2022 Valtteri Koskivuori. All rights reserved.
//

#include <stdio.h>
#include "../nodebase.h"

#include "../../../common/hashtable.h"
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

static void dump(const void *node, char *dumpbuf, int len) {
	struct blackbodyNode *self = (struct blackbodyNode *)node;
	char temperature[DUMPBUF_SIZE / 2] = "";
	if (self->temperature->base.dump) self->temperature->base.dump(self->temperature, temperature, sizeof(temperature));
	snprintf(dumpbuf, len, "blackbodyNode { input: %s }", temperature);
}

static struct color eval(const struct colorNode *node, sampler *sampler, const struct hitRecord *record) {
	(void)record;
	(void)sampler;
	struct blackbodyNode *this = (struct blackbodyNode *)node;
	return colorForKelvin(this->temperature->eval(this->temperature, sampler, record));
}

const struct colorNode *newBlackbody(const struct node_storage *s, const struct valueNode *temperature) {
	HASH_CONS(s->node_table, hash, struct blackbodyNode, {
		.temperature = temperature ? temperature : newConstantValue(s, 4000.0f),
		.node = {
			.eval = eval,
			.base = { .compare = compare, .dump = dump }
		}
	});
}
