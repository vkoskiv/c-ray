//
//  vecmix.c
//  C-Ray
//
//  Created by Valtteri Koskivuori on 28/12/2022.
//  Copyright © 2022 Valtteri Koskivuori. All rights reserved.
//

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
static struct vectorValue eval(const struct vectorNode *node, sampler *sampler, const struct hitRecord *record) {
	struct vec_mix *this = (struct vec_mix *)node;
	
	const float lerp = this->f->eval(this->f, record);

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
		.A = A ? A : newConstantVector(s, vecZero()),
		.B = B ? B : newConstantVector(s, vecZero()),
		.f = f ? f : newConstantValue(s, 0.0f),
		.node = {
			.eval = eval,
			.base = { .compare = compare }
		}
	});
}
