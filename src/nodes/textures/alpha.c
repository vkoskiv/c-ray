//
//  alpha.c
//  C-Ray
//
//  Created by Valtteri Koskivuori on 23/12/2020.
//  Copyright © 2020-2022 Valtteri Koskivuori. All rights reserved.
//

#include <stdio.h>
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

static void dump(const void *node, char *dumpbuf, int bufsize) {
	struct alphaNode *self = (struct alphaNode *)node;
	char color[DUMPBUF_SIZE / 2] = "";
	if (self->color->base.dump) self->color->base.dump(self->color, &color[0], sizeof(color));
	snprintf(dumpbuf, bufsize, "alphaNode { color: %s }", color);
}

static float eval(const struct valueNode *node, sampler *sampler, const struct hitRecord *record) {
	struct alphaNode *this = (struct alphaNode *)node;
	return this->color->eval(this->color, sampler, record).alpha;
}

const struct valueNode *newAlpha(const struct node_storage *s, const struct colorNode *color) {
	HASH_CONS(s->node_table, hash, struct alphaNode, {
		.color = color ? color : newConstantTexture(s, g_white_color),
		.node = {
			.eval = eval,
			.base = { .compare = compare, .dump = dump }
		}
	});
}
