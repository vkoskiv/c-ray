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

struct textureNode *newConstantTexture(struct block **pool, struct color color) {
	struct constantTexture *new = allocBlock(pool, sizeof(*new));
	new->color = color;
	new->node.eval = evalConstant;
	return (struct textureNode *)new;
}
