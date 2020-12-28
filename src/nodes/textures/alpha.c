//
//  alpha.c
//  C-Ray
//
//  Created by Valtteri Koskivuori on 23/12/2020.
//  Copyright © 2020 Valtteri Koskivuori. All rights reserved.
//

#include "../../includes.h"
#include "../../datatypes/color.h"
#include "../../datatypes/vertexbuffer.h"
#include "../../utils/assert.h"
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

const struct valueNode *newAlpha(const struct world *world, const struct colorNode *color) {
	HASH_CONS(world->nodeTable, hash, struct alphaNode, {
		.color = color ? color : newConstantTexture(world, whiteColor),
		.node = {
			.eval = eval,
			.base = { .compare = compare }
		}
	});
}
