//
//  grayscale.c
//  C-Ray
//
//  Created by Valtteri Koskivuori on 15/12/2020.
//  Copyright © 2020-2022 Valtteri Koskivuori. All rights reserved.
//

#include <stdio.h>
#include "../../datatypes/color.h"
#include "../../datatypes/poly.h"
#include "../../utils/mempool.h"
#include "../../datatypes/hitrecord.h"
#include "../../utils/hashtable.h"
#include "../../datatypes/scene.h"
#include "../valuenode.h"
#include "../colornode.h"

#include "grayscale.h"

struct grayscale {
	struct valueNode node;
	const struct colorNode *input;
};

static bool compare(const void *A, const void *B) {
	const struct grayscale *this = A;
	const struct grayscale *other = B;
	return this->input == other->input;
}

static uint32_t hash(const void *p) {
	const struct grayscale *this = p;
	uint32_t h = hashInit();
	h = hashBytes(h, &this->input, sizeof(this->input));
	return h;
}

static float eval(const struct valueNode *node, sampler *sampler, const struct hitRecord *record) {
	const struct grayscale *this = (struct grayscale *)node;
	return colorToGrayscale(this->input->eval(this->input, sampler, record)).red;
}

static void dump(const void *node, char *dumpbuf) {
	struct grayscale *self = (struct grayscale *)node;
	char color[DUMPBUF_SIZE / 2] = "";
	if (self->input->base.dump) self->input->base.dump(self->input, &color[0]);
	snprintf(dumpbuf, DUMPBUF_SIZE, "grayscale { input: %s }", color);
}

const struct valueNode *newGrayscaleConverter(const struct node_storage *s, const struct colorNode *node) {
	HASH_CONS(s->node_table, hash, struct grayscale, {
		.input = node ? node : newConstantTexture(s, g_black_color),
		.node = {
			.eval = eval,
			.base = { .compare = compare, .dump = dump }
		}
	});
}
