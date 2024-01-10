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

#include "hsv_transform.h"
#include "../../nodes/textures/constant.h"

struct HSVTransform {
	struct colorNode node;
	const struct colorNode *tex;
	const struct valueNode *H;
	const struct valueNode *S;
	const struct valueNode *V;
	const struct valueNode *f;
};

static bool compare(const void *A, const void *B) {
	const struct HSVTransform *this = A;
	const struct HSVTransform *other = B;
	return this->tex == other->tex &&
			this->H == other->H &&
			this->S == other->S &&
			this->V == other->V &&
			this->f == other->f;
}

static uint32_t hash(const void *p) {
	const struct HSVTransform *this = p;
	uint32_t h = hashInit();
	h = hashBytes(h, &this->tex, sizeof(this->tex));
	h = hashBytes(h, &this->H, sizeof(this->H));
	h = hashBytes(h, &this->S, sizeof(this->S));
	h = hashBytes(h, &this->V, sizeof(this->V));
	h = hashBytes(h, &this->f, sizeof(this->f));
	return h;
}

static void dump(const void *node, char *dumpbuf, int bufsize) {
	struct HSVTransform *self = (struct HSVTransform *)node;
	char tex[DUMPBUF_SIZE / 4] = "";
	char H[DUMPBUF_SIZE / 4] = "";
	char S[DUMPBUF_SIZE / 4] = "";
	char V[DUMPBUF_SIZE / 4] = "";
	char f[DUMPBUF_SIZE / 4] = "";
	if (self->tex->base.dump) self->tex->base.dump(self->tex, &tex[0], sizeof(tex));
	if (self->H->base.dump) self->H->base.dump(self->H, &H[0], sizeof(H));
	if (self->S->base.dump) self->S->base.dump(self->S, &S[0], sizeof(S));
	if (self->V->base.dump) self->V->base.dump(self->V, &V[0], sizeof(V));
	if (self->f->base.dump) self->f->base.dump(self->f, &f[0], sizeof(f));
	snprintf(dumpbuf, bufsize, "combineHSV { tex: %s, H: %s, S: %s, V: %s, f: %s }", tex, H, S, V, f);
}

static inline float saturatef(float in) {
	return 0.0f < in ? (in < 1.0f ? in : 1.0f) : 0.0f;
}

static struct color eval(const struct colorNode *node, sampler *sampler, const struct hitRecord *record) {
	const struct HSVTransform *this = (struct HSVTransform *)node;
	const struct color in_color = this->tex->eval(this->tex, sampler, record);
	const float h = this->H->eval(this->H, sampler, record);
	const float s = this->S->eval(this->S, sampler, record);
	const float v = this->V->eval(this->V, sampler, record);
	const float f = this->f->eval(this->f, sampler, record);

	struct hsv in = rgb_to_hsv(in_color);

	// FIXME: Probably a good idea to convert these ranges in driver parser and stick to [0,1] here
	in.h = in.h / 360.0f;
	in.s = in.s / 100.0f;
	in.v = in.v / 100.0f;

	struct hsv temp = (struct hsv){
		.h = fmodf(in.h + h + 0.5f, 1.0f),
		.s = saturatef(in.s * s),
		.v = in.v * v
	};

	temp.h = temp.h * 360.0f;
	temp.s = temp.s * 100.0f;
	temp.v = temp.v * 100.0f;
	
	struct color out = hsv_to_rgb(temp);
	out.red = f * out.red + (1.0f - f) * in_color.red;
	out.green = f * out.green + (1.0f - f) * in_color.green;
	out.blue = f * out.blue + (1.0f - f) * in_color.blue;
	return (struct color){ max(out.red, 0.0f), max(out.green, 0.0f), max(out.blue, 0.0f), 1.0f };
}

const struct colorNode *newHSVTransform(const struct node_storage *s, const struct colorNode *tex, const struct valueNode *H, const struct valueNode *S, const struct valueNode *V, const struct valueNode *f) {
	HASH_CONS(s->node_table, hash, struct HSVTransform, {
		.tex = tex ? tex : newConstantTexture(s, g_white_color),
		.H = H ? H : newConstantValue(s, 0.5f),
		.S = S ? S : newConstantValue(s, 1.0f),
		.V = V ? V : newConstantValue(s, 1.0f),
		.f = f ? f : newConstantValue(s, 1.0f),
		.node = {
				.eval = eval,
				.base = { .compare = compare, .dump = dump }
		}
	});
}
