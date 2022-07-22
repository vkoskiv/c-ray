//
//  vecmath.c
//  C-Ray
//
//  Created by Valtteri Koskivuori on 28/12/2020.
//  Copyright © 2020-2022 Valtteri Koskivuori. All rights reserved.
//

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
		case VecSubtract:
			return (struct vectorValue){ .v = vecSub(a, b) };
		case VecMultiply:
			return (struct vectorValue){ .v = vecMul(a, b) };
		case VecAverage:
			return (struct vectorValue){ .v = vecScale(vecAdd(a, b), 0.5f) };
		case VecDot:
			return (struct vectorValue){ .f = vecDot(a, b) };
		case VecCross:
			return (struct vectorValue){ .v = vecCross(a, b) };
		case VecNormalize:
			return (struct vectorValue){ .v = vecNormalize(a) };
		case VecReflect:
			return (struct vectorValue){ .v = vecReflect(a, b) };
		case VecLength:
			return (struct vectorValue){ .f = vecLength(a) };
		case VecAbs:
			return (struct vectorValue){ .v = { .x = fabsf(a.x), .y = fabsf(a.y), .z = fabsf(a.z) } };
		case VecMin:
			return (struct vectorValue){ .v = { .x = fminf(a.x, b.x), .y = fminf(a.y, b.y), .z = fminf(a.z, b.z) } };
		case VecMax:
			return (struct vectorValue){ .v = { .x = fmaxf(a.x, b.x), .y = fmaxf(a.y, b.y), .z = fmaxf(a.z, b.z) } };
		case VecFloor:
			return (struct vectorValue){ .v = { .x = floorf(a.x), .y = floorf(a.y), .z = floorf(a.z) } };
		case VecCeil:
			return (struct vectorValue){ .v = { .x = ceilf(a.x), .y = ceilf(a.y), .z = ceilf(a.z) } };
		case VecSin:
			return (struct vectorValue){ .v = { .x = sinf(a.x), .y = sinf(a.y), .z = sinf(a.z) } };
		case VecCos:
			return (struct vectorValue){ .v = { .x = cosf(a.x), .y = cosf(a.y), .z = cosf(a.z) } };
		case VecTan:
			return (struct vectorValue){ .v = { .x = tanf(a.x), .y = tanf(a.y), .z = tanf(a.z) } };
		case VecModulo:
			return (struct vectorValue){ .v = { .x = fmodf(a.x, b.x), .y = fmodf(a.y, b.y), .z = fmodf(a.z, b.z) } };
		case VecDistance:
			return (struct vectorValue){ .f = vecDistanceBetween(a, b) };
	}
	ASSERT_NOT_REACHED();
	return (struct vectorValue){ .v = { 0 }, .c = { 0 }, .f = 0.0f };
}

const struct vectorNode *newVecMath(const struct node_storage *s, const struct vectorNode *A, const struct vectorNode *B, const enum vecOp op) {
	HASH_CONS(s->node_table, hash, struct vecMathNode, {
		.A = A ? A : newConstantVector(s, vecZero()),
		.B = B ? B : newConstantVector(s, vecZero()),
		.op = op,
		.node = {
			.eval = eval,
			.base = { .compare = compare }
		}
	});
}
