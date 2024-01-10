//
//  combinehsl.c
//  c-ray
//
//  Created by Valtteri Koskivuori on 20/07/2022.
//  Copyright © 2022 Valtteri Koskivuori. All rights reserved.
//

#include <stdio.h>
#include "../../../common/color.h"
#include "../../../common/mempool.h"
#include "../../../common/hashtable.h"
#include "../../datatypes/hitrecord.h"
#include "../../datatypes/scene.h"

#include "combinehsl.h"
#include "../../nodes/textures/constant.h"

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

static void dump(const void *node, char *dumpbuf, int bufsize) {
	struct combineHSL *self = (struct combineHSL *)node;
	char H[DUMPBUF_SIZE / 4] = "";
	char S[DUMPBUF_SIZE / 4] = "";
	char L[DUMPBUF_SIZE / 4] = "";
	if (self->H->base.dump) self->H->base.dump(self->H, &H[0], sizeof(H));
	if (self->S->base.dump) self->S->base.dump(self->S, &S[0], sizeof(S));
	if (self->L->base.dump) self->L->base.dump(self->L, &L[0], sizeof(L));
	snprintf(dumpbuf, bufsize, "combineHSL { H: %s, S: %s, L: %s }", H, S, L);
}

static struct color eval(const struct colorNode *node, sampler *sampler, const struct hitRecord *record) {
	const struct combineHSL *this = (struct combineHSL *)node;
	return hsl_to_rgb((struct hsl){ this->H->eval(this->H, sampler, record), this->S->eval(this->S, sampler, record), this->L->eval(this->L, sampler, record) });
}

const struct colorNode *newCombineHSL(const struct node_storage *s, const struct valueNode *H, const struct valueNode *S, const struct valueNode *L) {
	if (H->constant && S->constant && L->constant) {
		float hue = H->eval(H, NULL, NULL);
		float sat = S->eval(S, NULL, NULL);
		float lig = L->eval(L, NULL, NULL);
		return newConstantTexture(s, hsl_to_rgb((struct hsl){ hue, sat, lig }));
	}
	HASH_CONS(s->node_table, hash, struct combineHSL, {
		.H = H ? H : newConstantValue(s, 0.0f),
		.S = S ? S : newConstantValue(s, 0.0f),
		.L = L ? L : newConstantValue(s, 0.0f),
		.node = {
				.eval = eval,
				.base = { .compare = compare, .dump = dump }
		}
	});
}
