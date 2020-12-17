//
//  combinergb.c
//  C-Ray
//
//  Created by Valtteri Koskivuori on 17/12/2020.
//  Copyright © 2020 Valtteri Koskivuori. All rights reserved.
//

#include "../../includes.h"
#include "../../datatypes/color.h"
#include "../../utils/assert.h"
#include "../../utils/mempool.h"
#include "../../datatypes/hitrecord.h"
#include "../../utils/hashtable.h"
#include "../../datatypes/scene.h"
#include "../valuenode.h"

#include "combinergb.h"

struct combineRGB {
	struct colorNode node;
	const struct valueNode *R;
	const struct valueNode *G;
	const struct valueNode *B;
};

static bool compare(const void *A, const void *B) {
	const struct combineRGB *this = A;
	const struct combineRGB *other = B;
	return this->R == other->R && this->G == other->G && this->B == other->B;
}

static uint32_t hash(const void *p) {
	const struct combineRGB *this = p;
	uint32_t h = hashInit();
	h = hashBytes(h, &this->R, sizeof(this->R));
	h = hashBytes(h, &this->G, sizeof(this->G));
	h = hashBytes(h, &this->B, sizeof(this->B));
	return h;
}

static struct color eval(const struct colorNode *node, const struct hitRecord *record) {
	const struct combineRGB *this = (struct combineRGB *)node;
	//TODO: What do we do with the alpha here?
	return (struct color){
		.red = this->R->eval(this->R, record),
		.green = this->G->eval(this->G, record),
		.blue = this->B->eval(this->B, record),
		1.0f
	};
}

struct colorNode *newCombineRGB(struct world *world, const struct valueNode *R, const struct valueNode *G, const struct valueNode *B) {
	HASH_CONS(world->nodeTable, world->nodePool, hash, struct combineRGB, {
		.R = R,
		.G = G,
		.B = B,
		.node = {
			.eval = eval,
			.base = { .compare = compare }
		}
	});
}
