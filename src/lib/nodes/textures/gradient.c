//
//  gradient.c
//  c-ray
//
//  Created by Valtteri Koskivuori on 19/12/2020.
//  Copyright © 2020-2025 Valtteri Koskivuori. All rights reserved.
//

#include <stdio.h>
#include <common/color.h>
#include <common/mempool.h>
#include <common/hashtable.h>
#include <datatypes/hitrecord.h>
#include <datatypes/scene.h>
#include "../colornode.h"

#include "gradient.h"

struct gradientTexture {
	struct colorNode node;
	struct color down;
	struct color up;
};

static bool compare(const void *A, const void *B) {
	const struct gradientTexture *this = A;
	const struct gradientTexture *other = B;
	return colorEquals(this->down, other->down) && colorEquals(this->up, other->up);
}

static uint32_t hash(const void *p) {
	const struct gradientTexture *this = p;
	uint32_t h = hashInit();
	h = hashBytes(h, &this->down, sizeof(this->down));
	h = hashBytes(h, &this->up, sizeof(this->up));
	return h;
}

static void dump(const void *node, char *dumpbuf, int len) {
	struct gradientTexture *self = (struct gradientTexture *)node;
	char down[64] = "";
	color_dump(self->down, &down[0], 64);
	char up[64] = "";
	color_dump(self->up, &up[0], 64);
	snprintf(dumpbuf, len, "gradientTexture { down: %s, up: %s }", down, up);
}

//Linearly interpolate based on the Y component
static struct color eval(const struct colorNode *node, sampler *sampler, const struct hitRecord *record) {
	(void)sampler;
	struct gradientTexture *this = (struct gradientTexture *)node;
	struct vector unitDir = vec_normalize(record->incident->direction);
	float t = 0.5f * (unitDir.y + 1.0f);
	return colorAdd(colorCoef(1.0f - t, this->down), colorCoef(t, this->up));
}

const struct colorNode *newGradientTexture(const struct node_storage *s, struct color down, struct color up) {
	HASH_CONS(s->node_table, hash, struct gradientTexture, {
		.down = down,
		.up = up,
		.node = {
			.eval = eval,
			.base = { .compare = compare, .dump = dump }
		}
	});
}
