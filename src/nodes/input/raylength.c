//
//  raylength.c
//  C-Ray
//
//  Created by Valtteri Koskivuori on 21/12/2020.
//  Copyright © 2020-2022 Valtteri Koskivuori. All rights reserved.
//

#include "../../datatypes/vector.h"
#include "../../datatypes/hitrecord.h"
#include "../../datatypes/scene.h"
#include "../../datatypes/vector.h"
#include "../../utils/hashtable.h"
#include "../bsdfnode.h"

#include "raylength.h"

struct rayLengthNode {
	struct valueNode node;
};

static bool compare(const void *A, const void *B) {
	(void)A;
	(void)B;
	return true;
}

static uint32_t hash(const void *p) {
	const struct rayLengthNode *this = p;
	uint32_t h = hashInit();
	h = hashBytes(h, &this->node.eval, sizeof(this->node.eval));
	return h;
}

static float eval(const struct valueNode *node, const struct hitRecord *record) {
	(void)record;
	(void)node;
	return record->distance;
}

const struct valueNode *newRayLength(const struct node_storage *s) {
	HASH_CONS(s->node_table, hash, struct rayLengthNode, {
		.node = {
			.eval = eval,
			.base = { .compare = compare }
		}
	});
}
