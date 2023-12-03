//
//  transparent.c
//  C-Ray
//
//  Created by Valtteri Koskivuori on 16/12/2020.
//  Copyright © 2020-2022 Valtteri Koskivuori. All rights reserved.
//

#include <stdio.h>
#include "../../datatypes/color.h"
#include "../../renderer/samplers/sampler.h"
#include "../../datatypes/vector.h"
#include "../../datatypes/hitrecord.h"
#include "../colornode.h"
#include "../../utils/hashtable.h"
#include "../../datatypes/scene.h"
#include "../bsdfnode.h"

#include "transparent.h"

struct transparent {
	struct bsdfNode bsdf;
	const struct colorNode *color;
};

static bool compare(const void *A, const void *B) {
	const struct transparent *this = A;
	const struct transparent *other = B;
	return this->color == other->color;
}

static uint32_t hash(const void *p) {
	const struct transparent *thing = p;
	uint32_t h = hashInit();
	h = hashBytes(h, &thing->color, sizeof(thing->color));
	return h;
}

static void dump(const void *node, char *dumpbuf, int bufsize) {
	struct transparent *self = (struct transparent *)node;
	char color[DUMPBUF_SIZE / 2] = "";
	if (self->color->base.dump) self->color->base.dump(self->color, color, sizeof(color));
	snprintf(dumpbuf, bufsize, "transparent { color: %s }", color);
}

static struct bsdfSample sample(const struct bsdfNode *bsdf, sampler *sampler, const struct hitRecord *record) {
	(void)sampler;
	struct transparent *this = (struct transparent *)bsdf;
	return (struct bsdfSample){ .out = record->incident_dir, .weight = this->color->eval(this->color, sampler, record) };
}

const struct bsdfNode *newTransparent(const struct node_storage *s, const struct colorNode *color) {
	HASH_CONS(s->node_table, hash, struct transparent, {
		.color = color ? color : newConstantTexture(s, g_white_color),
		.bsdf = {
			.sample = sample,
			.base = { .compare = compare, .dump = dump }
		}
	});
}
