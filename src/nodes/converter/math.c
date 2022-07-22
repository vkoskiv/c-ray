//
//  math.c
//  C-Ray
//
//  Created by Valtteri Koskivuori on 28/12/2020.
//  Copyright © 2020-2022 Valtteri Koskivuori. All rights reserved.
//

#include "../nodebase.h"

#include "../../utils/hashtable.h"
#include "../../datatypes/scene.h"
#include "../../datatypes/hitrecord.h"
#include "../../datatypes/transforms.h"

#include "math.h"

struct mathNode {
	struct valueNode node;
	const struct valueNode *A;
	const struct valueNode *B;
	const enum mathOp op;
};

static bool compare(const void *A, const void *B) {
	const struct mathNode *this = A;
	const struct mathNode *other = B;
	return this->A == other->A && this->B == other->B && this->op == other->op;
}

static uint32_t hash(const void *p) {
	const struct mathNode *this = p;
	uint32_t h = hashInit();
	h = hashBytes(h, &this->A, sizeof(this->A));
	h = hashBytes(h, &this->B, sizeof(this->B));
	h = hashBytes(h, &this->op, sizeof(this->op));
	return h;
}

static float eval(const struct valueNode *node, const struct hitRecord *record) {
	struct mathNode *this = (struct mathNode *)node;
	const float a = this->A->eval(this->A, record);
	const float b = this->B->eval(this->B, record);
	switch (this->op) {
		case Add:
			return a + b;
			break;
		case Subtract:
			return a - b;
			break;
		case Multiply:
			return a * b;
			break;
		case Divide:
			return a / b;
			break;
		case Power:
			return powf(a, b);
			break;
		case Log:
			return log10f(a);
			break;
		case SquareRoot:
			return sqrtf(a);
			break;
		case Absolute:
			return fabsf(a);
			break;
		case Min:
			return min(a, b);
			break;
		case Max:
			return max(a, b);
			break;
		case Sine:
			return sinf(a);
			break;
		case Cosine:
			return cosf(a);
			break;
		case Tangent:
			return tanf(a);
			break;
		case ToRadians:
			return toRadians(a);
			break;
		case ToDegrees:
			return fromRadians(a);
			break;
	}
	ASSERT_NOT_REACHED();
	return 0.0f;
}

const struct valueNode *newMath(const struct node_storage *s, const struct valueNode *A, const struct valueNode *B, const enum mathOp op) {
	HASH_CONS(s->node_table, hash, struct mathNode, {
		.A = A ? A : newConstantValue(s, 0.0f),
		.B = B ? B : newConstantValue(s, 0.0f),
		.op = op,
		.node = {
			.eval = eval,
			.base = { .compare = compare }
		}
	});
}
