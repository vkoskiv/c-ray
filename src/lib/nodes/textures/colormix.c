//
//  colormix.c
//  c-ray
//
//  Created by Valtteri Koskivuori on 17/12/2023.
//  Copyright © 2023 Valtteri Koskivuori. All rights reserved.
//

#include <stdio.h>
#include "../../../common/color.h"
#include "../../../common/hashtable.h"
#include "../../datatypes/scene.h"
#include "../valuenode.h"
#include "../colornode.h"

#include "colormix.h"

struct color_mix {
	struct colorNode node;
	const struct colorNode *A;
	const struct colorNode *B;
	const struct valueNode *f;
};

static bool compare(const void *A, const void *B) {
	struct color_mix *this = (struct color_mix *)A;
	struct color_mix *other = (struct color_mix *)B;
	return this->A == other->A && this->B == other->B && this->f == other->f;
}

static uint32_t hash(const void *p) {
	const struct color_mix *this = p;
	uint32_t h = hashInit();
	h = hashBytes(h, &this->A, sizeof(this->A));
	h = hashBytes(h, &this->B, sizeof(this->B));
	h = hashBytes(h, &this->f, sizeof(this->f));
	return h;
}

static void dump(const void *node, char *dumpbuf, int bufsize) {
	struct color_mix *self = (struct color_mix *)node;
	char A[DUMPBUF_SIZE / 2] = "";
	char B[DUMPBUF_SIZE / 2] = "";
	char f[DUMPBUF_SIZE / 2] = "";
	if (self->A->base.dump) self->A->base.dump(self->A, A, sizeof(A));
	if (self->B->base.dump) self->B->base.dump(self->B, B, sizeof(B));
	if (self->f->base.dump) self->f->base.dump(self->f, f, sizeof(f));
	snprintf(dumpbuf, bufsize, "color_mix { A: %s, B: %s, f: %s }", A, B, f);
}

static struct color eval(const struct colorNode *node, sampler *sampler, const struct hitRecord *record) {
	struct color_mix *this = (struct color_mix *)node;

	const float lerp = this->f->eval(this->f, sampler, record);

	if (getDimension(sampler) > lerp) {
		return this->A->eval(this->A, sampler, record);
	} else {
		return this->B->eval(this->B, sampler, record);
	}
}

const struct colorNode *new_color_mix(const struct node_storage *s, const struct colorNode *A, const struct colorNode *B, const struct valueNode *f) {
	if (A == B) {
		logr(debug, "A == B, pruning color_mix node.\n");
		return A;
	}
	HASH_CONS(s->node_table, hash, struct color_mix, {
		.A = A ? A : newConstantTexture(s, g_black_color),
		.B = B ? B : newConstantTexture(s, g_black_color),
		.f = f ? f : newConstantValue(s, 0.0f),
		.node = {
			.eval = eval,
			.base = { .compare = compare, .dump = dump }
		}
	});
}

