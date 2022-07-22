//
//  combinehsl.c
//  c-ray
//
//  Created by Valtteri Koskivuori on 20/07/2022.
//  Copyright © 2022 Valtteri Koskivuori. All rights reserved.
//

#include "../../datatypes/color.h"
#include "../../utils/mempool.h"
#include "../../datatypes/hitrecord.h"
#include "../../utils/hashtable.h"
#include "../../datatypes/scene.h"

#include "combinehsl.h"

struct combineHSL {
	struct colorNode node;
	const struct valueNode *H;
	const struct valueNode *S;
	const struct valueNode *L;
};

static bool compare(const void *A, const void *B) {
	const struct combineHSL *this = A;
	const struct combineHSL *other = B;
	return this->H == other->H && this->S == other->S && this->L == other->L;
}

static uint32_t hash(const void *p) {
	const struct combineHSL *this = p;
	uint32_t h = hashInit();
	h = hashBytes(h, &this->H, sizeof(this->H));
	h = hashBytes(h, &this->S, sizeof(this->S));
	h = hashBytes(h, &this->L, sizeof(this->L));
	return h;
}

static struct color eval(const struct colorNode *node, const struct hitRecord *record) {
	const struct combineHSL *this = (struct combineHSL *)node;
	return color_from_hsl(this->H->eval(this->H, record), this->S->eval(this->S, record), this->L->eval(this->L, record));
}

const struct colorNode *newCombineHSL(const struct node_storage *s, const struct valueNode *H, const struct valueNode *S, const struct valueNode *L) {
	HASH_CONS(s->node_table, hash, struct combineHSL, {
		.H = H ? H : newConstantValue(s, 0.0f),
		.S = S ? S : newConstantValue(s, 0.0f),
		.L = L ? L : newConstantValue(s, 0.0f),
		.node = {
				.eval = eval,
				.base = { .compare = compare }
		}
	});
}