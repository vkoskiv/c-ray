//
//  normal.c
//  C-Ray
//
//  Created by Valtteri Koskivuori on 20/12/2020.
//  Copyright © 2020-2022 Valtteri Koskivuori. All rights reserved.
//

#include <stdio.h>
#include "../../renderer/samplers/sampler.h"
#include "../../../common/color.h"
#include "../../../common/vector.h"
#include "../../../common/vector.h"
#include "../../../common/hashtable.h"
#include "../../datatypes/hitrecord.h"
#include "../../datatypes/scene.h"
#include "../bsdfnode.h"

#include "normal.h"

struct normalNode {
	struct vectorNode node;
};

static bool compare(const void *A, const void *B) {
	(void)A;
	(void)B;
	return true;
}

static uint32_t hash(const void *p) {
	const struct normalNode *this = p;
	uint32_t h = hashInit();
	h = hashBytes(h, &this->node.eval, sizeof(this->node.eval));
	return h;
}

static void dump(const void *node, char *dumpbuf, int bufsize) {
	(void)node;
	snprintf(dumpbuf, bufsize, "normalNode { }");
}

static union vector_value eval(const struct vectorNode *node, sampler *sampler, const struct hitRecord *record) {
	(void)record;
	(void)node;
	(void)sampler;
	return (union vector_value){ .v = record->surfaceNormal };
}

const struct vectorNode *newNormal(const struct node_storage *s) {
	HASH_CONS(s->node_table, hash, struct normalNode, {
		.node = {
			.eval = eval,
			.base = { .compare = compare, .dump = dump }
		}
	});
}
