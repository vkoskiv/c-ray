//
//  normal.c
//  C-Ray
//
//  Created by Valtteri Koskivuori on 20/12/2020.
//  Copyright © 2020-2022 Valtteri Koskivuori. All rights reserved.
//

#include "../../datatypes/color.h"
#include "../../datatypes/vector.h"
#include "../../datatypes/hitrecord.h"
#include "../../datatypes/scene.h"
#include "../../datatypes/vector.h"
#include "../../utils/hashtable.h"
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

static struct vectorValue eval(const struct vectorNode *node, const struct hitRecord *record) {
	(void)record;
	(void)node;
	return (struct vectorValue){ .v = record->surfaceNormal, .c = coordZero() };
}

const struct vectorNode *newNormal(const struct node_storage *s) {
	HASH_CONS(s->node_table, hash, struct normalNode, {
		.node = {
			.eval = eval,
			.base = { .compare = compare }
		}
	});
}
