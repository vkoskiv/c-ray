//
//  add.c
//  C-Ray
//
//  Created by Valtteri Koskivuori on 17/12/2020.
//  Copyright © 2020-2022 Valtteri Koskivuori. All rights reserved.
//

#include <stdio.h>
#include "../../datatypes/color.h"
#include "../../renderer/samplers/sampler.h"
#include "../../datatypes/vector.h"
#include "../colornode.h"
#include "../../utils/hashtable.h"
#include "../../datatypes/scene.h"
#include "../bsdfnode.h"

#include "add.h"

struct addBsdf {
	struct bsdfNode bsdf;
	const struct bsdfNode *A;
	const struct bsdfNode *B;
};

static bool compare(const void *A, const void *B) {
	struct addBsdf *this = (struct addBsdf *)A;
	struct addBsdf *other = (struct addBsdf *)B;
	return this->A == other->A && this->B == other->B;
}

static uint32_t hash(const void *p) {
	const struct addBsdf *this = p;
	uint32_t h = hashInit();
	h = hashBytes(h, &this->A, sizeof(this->A));
	h = hashBytes(h, &this->B, sizeof(this->B));
	return h;
}

static void dump(const void *node, char *dumpbuf, int bufsize) {
	struct addBsdf *self = (struct addBsdf *)node;
	char A[DUMPBUF_SIZE / 2] = "";
	char B[DUMPBUF_SIZE / 2] = "";
	if (self->A->base.dump) self->A->base.dump(self->A, A, sizeof(A));
	if (self->B->base.dump) self->B->base.dump(self->B, B, sizeof(B));
	snprintf(dumpbuf, bufsize, "addBsdf { A: %s, B: %s }", A, B);
}

static struct bsdfSample sample(const struct bsdfNode *bsdf, sampler *sampler, const struct hitRecord *record) {
	struct addBsdf *mixBsdf = (struct addBsdf *)bsdf;
	struct bsdfSample A = mixBsdf->A->sample(mixBsdf->A, sampler, record);
	struct bsdfSample B = mixBsdf->B->sample(mixBsdf->B, sampler, record);
	//TODO: Do we just add the outgoing vertices together or what...?
	//TODO: Find out if we want to even keep this node around.
	return (struct bsdfSample){.out = vec_add(A.out, B.out), .weight = colorAdd(A.weight, B.weight)};
}

const struct bsdfNode *newAdd(const struct node_storage *s, const struct bsdfNode *A, const struct bsdfNode *B) {
	if (A == B) {
		logr(debug, "A == B, pruning add node.\n");
		return A;
	}
	HASH_CONS(s->node_table, hash, struct addBsdf, {
		.A = A ? A : newDiffuse(s, newConstantTexture(s, g_black_color)),
		.B = B ? B : newDiffuse(s, newConstantTexture(s, g_black_color)),
		.bsdf = {
			.sample = sample,
			.base = { .compare = compare, .dump = dump }
		}
	});
}
