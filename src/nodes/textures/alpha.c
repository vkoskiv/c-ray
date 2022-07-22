//
//  alpha.c
//  C-Ray
//
//  Created by Valtteri Koskivuori on 23/12/2020.
//  Copyright © 2020-2022 Valtteri Koskivuori. All rights reserved.
//

#include "../../datatypes/color.h"
#include "../../datatypes/hitrecord.h"
#include "../../utils/hashtable.h"
#include "../../datatypes/scene.h"
#include "../colornode.h"

#include "alpha.h"

struct alphaNode {
	struct valueNode node;
	const struct colorNode *color;
};

static bool compare(const void *A, const void *B) {
	const struct alphaNode *this = A;
	const struct alphaNode *other = B;
	return this->color == other->color;
}

static uint32_t hash(const void *p) {
	const struct alphaNode *this = p;
	uint32_t h = hashInit();
	h = hashBytes(h, &this->color, sizeof(this->color));
	return h;
}

static float eval(const struct valueNode *node, const struct hitRecord *record) {
	struct alphaNode *this = (struct alphaNode *)node;
	return this->color->eval(this->color, record).alpha;
}

const struct valueNode *newAlpha(const struct node_storage *s, const struct colorNode *color) {
	HASH_CONS(s->node_table, hash, struct alphaNode, {
		.color = color ? color : newConstantTexture(s, whiteColor),
		.node = {
			.eval = eval,
			.base = { .compare = compare }
		}
	});
}
