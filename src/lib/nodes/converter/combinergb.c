//
//  combinergb.c
//  c-ray
//
//  Created by Valtteri Koskivuori on 17/12/2020.
//  Copyright © 2020-2025 Valtteri Koskivuori. All rights reserved.
//

#include <stdio.h>
#include <common/color.h>
#include <common/mempool.h>
#include <common/hashtable.h>
#include <datatypes/hitrecord.h>
#include <datatypes/scene.h>
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

static void dump(const void *node, char *dumpbuf, int bufsize) {
	struct combineRGB *self = (struct combineRGB *)node;
	char R[DUMPBUF_SIZE / 4] = "";
	char G[DUMPBUF_SIZE / 4] = "";
	char B[DUMPBUF_SIZE / 4] = "";
	if (self->R->base.dump) self->R->base.dump(self->R, &R[0], sizeof(R));
	if (self->G->base.dump) self->G->base.dump(self->G, &G[0], sizeof(G));
	if (self->B->base.dump) self->B->base.dump(self->B, &B[0], sizeof(B));
	snprintf(dumpbuf, bufsize, "combineRGB { R: %s, G: %s, B: %s }", R, G, B);
}

static struct color eval(const struct colorNode *node, sampler *sampler, const struct hitRecord *record) {
	const struct combineRGB *this = (struct combineRGB *)node;
	//TODO: What do we do with the alpha here?
	return (struct color){
		.red   = this->R->eval(this->R, sampler, record),
		.green = this->G->eval(this->G, sampler, record),
		.blue  = this->B->eval(this->B, sampler, record),
		1.0f
	};
}

const struct colorNode *newCombineRGB(const struct node_storage *s, const struct valueNode *R, const struct valueNode *G, const struct valueNode *B) {
	HASH_CONS(s->node_table, hash, struct combineRGB, {
		.R = R ? R : newConstantValue(s, 0.0f),
		.G = G ? G : newConstantValue(s, 0.0f),
		.B = B ? B : newConstantValue(s, 0.0f),
		.node = {
			.eval = eval,
			.base = { .compare = compare, .dump = dump }
		}
	});
}
