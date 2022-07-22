//
//  uv.c
//  C-Ray
//
//  Created by Valtteri Koskivuori on 09/08/2021.
//  Copyright © 2021-2022 Valtteri Koskivuori. All rights reserved.
//

#include "../../datatypes/color.h"
#include "../../datatypes/vector.h"
#include "../../datatypes/hitrecord.h"
#include "../../datatypes/scene.h"
#include "../../datatypes/vector.h"
#include "../../utils/hashtable.h"
#include "../bsdfnode.h"

#include "uv.h"

struct uvNode {
	struct vectorNode node;
};

static bool compare(const void *A, const void *B) {
	(void)A;
	(void)B;
	return true;
}

static uint32_t hash(const void *p) {
	const struct uvNode *this = p;
	uint32_t h = hashInit();
	h = hashBytes(h, &this->node.eval, sizeof(this->node.eval));
	return h;
}

static struct vectorValue eval(const struct vectorNode *node, const struct hitRecord *record) {
	(void)record;
	(void)node;
	return (struct vectorValue){ .v = vecZero(), .c = record->uv };
}

const struct vectorNode *newUV(const struct node_storage *s) {
	HASH_CONS(s->node_table, hash, struct uvNode, {
		.node = {
			.eval = eval,
			.base = { .compare = compare }
		}
	});
}
