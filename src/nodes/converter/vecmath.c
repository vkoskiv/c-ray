//
//  vecmath.c
//  C-Ray
//
//  Created by Valtteri Koskivuori on 28/12/2020.
//  Copyright © 2020 Valtteri Koskivuori. All rights reserved.
//

#include "../../includes.h"
#include "../nodebase.h"

#include "../../utils/hashtable.h"
#include "../../datatypes/scene.h"
#include "../../datatypes/hitrecord.h"
#include "../vectornode.h"

#include "vecmath.h"

struct vecMathNode {
	struct vectorNode node;
	const struct vectorNode *A;
	const struct vectorNode *B;
	const enum vecOp op;
};

static bool compare(const void *A, const void *B) {
	const struct vecMathNode *this = A;
	const struct vecMathNode *other = B;
	return this->A == other->A && this->B == other->B && this->op == other->op;
}

static uint32_t hash(const void *p) {
	const struct vecMathNode *this = p;
	uint32_t h = hashInit();
	h = hashBytes(h, &this->A, sizeof(this->A));
	h = hashBytes(h, &this->B, sizeof(this->B));
	h = hashBytes(h, &this->op, sizeof(this->op));
	return h;
}
 
static struct vectorValue eval(const struct vectorNode *node, const struct hitRecord *record) {
	struct vecMathNode *this = (struct vecMathNode *)node;
	
	const struct vector a = this->A->eval(this->A, record).v;
	const struct vector b = this->B->eval(this->B, record).v;
	
	switch (this->op) {
		case VecAdd:
			return (struct vectorValue){ .v = vecAdd(a, b) };
			break;
		case VecSubtract:
			return (struct vectorValue){ .v = vecSub(a, b) };
			break;
		case VecMultiply:
			return (struct vectorValue){ .v = vecMul(a, b) };
			break;
		case VecAverage:
			return (struct vectorValue){ .v = vecScale(vecAdd(a, b), 0.5f) };
			break;
		case VecDot:
			return (struct vectorValue){ .f = vecDot(a, b) };
			break;
		case VecCross:
			return (struct vectorValue){ .v = vecCross(a, b) };
			break;
		case VecNormalize:
			return (struct vectorValue){ .v = vecNormalize(a) };
			break;
		case VecReflect:
			return (struct vectorValue){ .v = vecReflect(a, b) };
			break;
		case VecLength:
			return (struct vectorValue){ .f = vecLength(a) };
			break;
	}
	ASSERT_NOT_REACHED();
	return (struct vectorValue){ .v = { 0 }, .c = { 0 }, .f = 0.0f };
}

const struct vectorNode *newVecMath(const struct world *world, const struct vectorNode *A, const struct vectorNode *B, const enum vecOp op) {
	HASH_CONS(world->nodeTable, hash, struct vecMathNode, {
		.A = A ? A : newConstantVector(world, vecZero()),
		.B = B ? B : newConstantVector(world, vecZero()),
		.op = op,
		.node = {
			.eval = eval,
			.base = { .compare = compare }
		}
	});
}
