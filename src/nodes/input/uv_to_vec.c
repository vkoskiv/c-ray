//
//  uv_to_vec.c
//  C-Ray
//
//  Created by Valtteri Koskivuori on 29/12/2022.
//  Copyright © 2022 Valtteri Koskivuori. All rights reserved.
//

#include "../../renderer/samplers/sampler.h"
#include "../../datatypes/color.h"
#include "../../datatypes/vector.h"
#include "../../datatypes/hitrecord.h"
#include "../../datatypes/scene.h"
#include "../../datatypes/vector.h"
#include "../../utils/hashtable.h"
#include "../bsdfnode.h"

#include "uv_to_vec.h"

struct uv_to_vec {
	struct vectorNode node;
	const struct vectorNode *A;
};

static bool compare(const void *A, const void *B) {
	const struct uv_to_vec *this = A;
	const struct uv_to_vec *other = B;
	return this->A == other->A;
}

static uint32_t hash(const void *p) {
	const struct uv_to_vec *this = p;
	uint32_t h = hashInit();
	h = hashBytes(h, &this->A, sizeof(this->A));
	return h;
}

static struct vectorValue eval(const struct vectorNode *node, sampler *sampler, const struct hitRecord *record) {
	const struct uv_to_vec *this = (struct uv_to_vec *)node;
	const struct vectorValue A = this->A->eval(this->A, sampler, record);
	return (struct vectorValue){ .v = { A.c.x, A.c.y, 0.0f }, .c = A.c };
}

const struct vectorNode *new_uv_to_vec(const struct node_storage *s, const struct vectorNode *A) {
	HASH_CONS(s->node_table, hash, struct uv_to_vec, {
		.A = A ? A : newConstantVector(s, vecZero()),
		.node = {
			.eval = eval,
			.base = { .compare = compare }
		}
	});
}
