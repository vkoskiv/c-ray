//
//  gradient.c
//  C-Ray
//
//  Created by Valtteri Koskivuori on 19/12/2020.
//  Copyright © 2020-2022 Valtteri Koskivuori. All rights reserved.
//

#include "../../datatypes/color.h"
#include "../../utils/mempool.h"
#include "../../datatypes/hitrecord.h"
#include "../../utils/hashtable.h"
#include "../../datatypes/scene.h"
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

//Linearly interpolate based on the Y component
static struct color eval(const struct colorNode *node, const struct hitRecord *record) {
	struct gradientTexture *this = (struct gradientTexture *)node;
	struct vector unitDir = vecNormalize(record->incident_dir);
	float t = 0.5f * (unitDir.y + 1.0f);
	return colorAdd(colorCoef(1.0f - t, this->down), colorCoef(t, this->up));
}

const struct colorNode *newGradientTexture(const struct node_storage *s, struct color down, struct color up) {
	HASH_CONS(s->node_table, hash, struct gradientTexture, {
		.down = down,
		.up = up,
		.node = {
			.eval = eval,
			.base = { .compare = compare }
		}
	});
}
