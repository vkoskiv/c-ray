//
//  combinehsv.c
//  c-ray
//
//  Created by Valtteri Koskivuori on 09/01/2024.
//  Copyright © 2024 Valtteri Koskivuori. All rights reserved.
//

#include <stdio.h>
#include "../../../common/color.h"
#include "../../../common/mempool.h"
#include "../../../common/hashtable.h"
#include "../../datatypes/hitrecord.h"
#include "../../datatypes/scene.h"

#include "combinehsl.h"
#include "../../nodes/textures/constant.h"

struct HSVTransform {
	struct colorNode node;
	const struct valueNode *H;
	const struct valueNode *S;
	const struct valueNode *V;
};

static bool compare(const void *A, const void *B) {
	const struct HSVTransform *this = A;
	const struct HSVTransform *other = B;
	return this->H == other->H && this->S == other->S && this->V == other->V;
}

static uint32_t hash(const void *p) {
	const struct HSVTransform *this = p;
	uint32_t h = hashInit();
	h = hashBytes(h, &this->H, sizeof(this->H));
	h = hashBytes(h, &this->S, sizeof(this->S));
	h = hashBytes(h, &this->V, sizeof(this->V));
	return h;
}

static void dump(const void *node, char *dumpbuf, int bufsize) {
	struct HSVTransform *self = (struct HSVTransform *)node;
	char H[DUMPBUF_SIZE / 4] = "";
	char S[DUMPBUF_SIZE / 4] = "";
	char V[DUMPBUF_SIZE / 4] = "";
	if (self->H->base.dump) self->H->base.dump(self->H, &H[0], sizeof(H));
	if (self->S->base.dump) self->S->base.dump(self->S, &S[0], sizeof(S));
	if (self->V->base.dump) self->V->base.dump(self->V, &V[0], sizeof(V));
	snprintf(dumpbuf, bufsize, "combineHSV { H: %s, S: %s, V: %s }", H, S, V);
}

static struct color eval(const struct colorNode *node, sampler *sampler, const struct hitRecord *record) {
	const struct HSVTransform *this = (struct HSVTransform *)node;
	return hsv_to_rgb((struct hsv){ this->H->eval(this->H, sampler, record), this->S->eval(this->S, sampler, record), this->V->eval(this->V, sampler, record) });
}

const struct colorNode *newCombineHSV(const struct node_storage *s, const struct valueNode *H, const struct valueNode *S, const struct valueNode *V) {
	if (H->constant && S->constant && V->constant) {
		float hue = H->eval(H, NULL, NULL);
		float sat = S->eval(S, NULL, NULL);
		float val = V->eval(V, NULL, NULL);
		return newConstantTexture(s, hsv_to_rgb((struct hsv){ hue, sat, val }));
	}
	HASH_CONS(s->node_table, hash, struct HSVTransform, {
		.H = H ? H : newConstantValue(s, 0.0f),
		.S = S ? S : newConstantValue(s, 0.0f),
		.V = V ? V : newConstantValue(s, 0.0f),
		.node = {
				.eval = eval,
				.base = { .compare = compare, .dump = dump }
		}
	});
}
