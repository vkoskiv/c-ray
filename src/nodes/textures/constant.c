//
//  constant.c
//  C-Ray
//
//  Created by Valtteri Koskivuori on 06/12/2020.
//  Copyright © 2020 Valtteri Koskivuori. All rights reserved.
//

#include "../../includes.h"
#include "../../datatypes/color.h"
#include "../../datatypes/poly.h"
#include "../../datatypes/vertexbuffer.h"
#include "../../utils/assert.h"
#include "../../datatypes/image/texture.h"
#include "../../utils/mempool.h"
#include "../../datatypes/hitrecord.h"
#include "../../utils/hashtable.h"
#include "../../datatypes/scene.h"
#include "texturenode.h"

#include "constant.h"

struct constantTexture {
	struct textureNode node;
	struct color color;
};

struct color evalConstant(const struct textureNode *node, const struct hitRecord *record) {
	(void)record;
	return ((struct constantTexture *)node)->color;
}

static bool compare(const void *A, const void *B) {
	const struct constantTexture *this = A;
	const struct constantTexture *other = B;
	return colorEquals(this->color, other->color);
}

static uint32_t hash(const void *p) {
	const struct constantTexture *this = p;
	uint32_t h = hashInit();
	h = hashBytes(h, &this->color, sizeof(this->color));
	return h;
}

struct textureNode *newConstantTexture(struct world *world, struct color color) {
	HASH_CONS(world->nodeTable, &world->nodePool, hash, struct constantTexture, {
		.color = color,
		.node = {
			.eval = evalConstant,
			.base = { .compare = compare }
		}
	});
}
