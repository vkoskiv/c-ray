//
//  constant.c
//  C-Ray
//
//  Created by Valtteri Koskivuori on 06/12/2020.
//  Copyright © 2020-2022 Valtteri Koskivuori. All rights reserved.
//

#include <stdio.h>
#include "../../datatypes/color.h"
#include "../../datatypes/poly.h"
#include "../../utils/mempool.h"
#include "../../datatypes/hitrecord.h"
#include "../../utils/hashtable.h"
#include "../../datatypes/scene.h"
#include "../colornode.h"

#include "constant.h"

struct constantTexture {
	struct colorNode node;
	struct color color;
};

static bool compare(const void *A, const void *B) {
	const struct constantTexture *this = A;
	const struct constantTexture *other = B;
	return colorEquals(this->color, other->color);
}

static uint32_t hash(const void *p) {
	const struct constantTexture *this = p;
	uint32_t h = hashInit();
	h = hashBytes(h, &this->color, sizeof(this->color));
	return h;
}

static void dump(const void *node, char *dumpbuf, int len) {
	struct constantTexture *self = (struct constantTexture *)node;
	char color[64] = "";
	color_dump(self->color, color, 64);
	snprintf(dumpbuf, len, "constantTexture { color: %s }", color);
}

static struct color eval(const struct colorNode *node, sampler *sampler, const struct hitRecord *record) {
	(void)record;
	(void)sampler;
	return ((struct constantTexture *)node)->color;
}

const struct colorNode *newConstantTexture(const struct node_storage *s, const struct color color) {
	HASH_CONS(s->node_table, hash, struct constantTexture, {
		.color = color,
		.node = {
			.eval = eval,
			.base = { .compare = compare, .dump = dump }
		}
	});
}
