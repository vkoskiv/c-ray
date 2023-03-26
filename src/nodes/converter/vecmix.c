//
//  vecmix.c
//  C-Ray
//
//  Created by Valtteri Koskivuori on 28/12/2022.
//  Copyright © 2022 Valtteri Koskivuori. All rights reserved.
//

#include <stdio.h>
#include "../../datatypes/vector.h"
#include "../../utils/hashtable.h"
#include "../../datatypes/scene.h"
#include "../valuenode.h"
#include "../vectornode.h"

#include "vecmix.h"

struct vec_mix {
	struct vectorNode node;
	const struct vectorNode *A;
	const struct vectorNode *B;
	const struct valueNode *f;
};

static bool compare(const void *A, const void *B) {
	struct vec_mix *this = (struct vec_mix *)A;
	struct vec_mix *other = (struct vec_mix *)B;
	return this->A == other->A && this->B == other->B && this->f == other->f;
}

static uint32_t hash(const void *p) {
	const struct vec_mix *this = p;
	uint32_t h = hashInit();
	h = hashBytes(h, &this->A, sizeof(this->A));
	h = hashBytes(h, &this->B, sizeof(this->B));
	h = hashBytes(h, &this->f, sizeof(this->f));
	return h;
}

static void dump(const void *node, char *dumpbuf, int bufsize) {
	struct vec_mix *self = (struct vec_mix *)node;
	char A[DUMPBUF_SIZE / 2] = "";
	char B[DUMPBUF_SIZE / 2] = "";
	char f[DUMPBUF_SIZE / 2] = "";
	if (self->A->base.dump) self->A->base.dump(self->A, A, sizeof(A));
	if (self->B->base.dump) self->B->base.dump(self->B, B, sizeof(B));
	if (self->f->base.dump) self->f->base.dump(self->f, f, sizeof(f));
	snprintf(dumpbuf, bufsize, "vec_mix { A: %s, B: %s, f: %s }", A, B, f);
}

static struct vectorValue eval(const struct vectorNode *node, sampler *sampler, const struct hitRecord *record) {
	struct vec_mix *this = (struct vec_mix *)node;
	
	const float lerp = this->f->eval(this->f, sampler, record);

	if (getDimension(sampler) > lerp) {
		return (struct vectorValue){ .v = this->A->eval(this->A, sampler, record).v };
	} else {
		return (struct vectorValue){ .v = this->B->eval(this->B, sampler, record).v };
	}
}

const struct vectorNode *new_vec_mix(const struct node_storage *s, const struct vectorNode *A, const struct vectorNode *B, const struct valueNode *f) {
	if (A == B) {
		logr(debug, "A == B, pruning vec_mix node.\n");
		return A;
	}
	HASH_CONS(s->node_table, hash, struct vec_mix, {
		.A = A ? A : newConstantVector(s, vec_zero()),
		.B = B ? B : newConstantVector(s, vec_zero()),
		.f = f ? f : newConstantValue(s, 0.0f),
		.node = {
			.eval = eval,
			.base = { .compare = compare, .dump = dump }
		}
	});
}
