//
//  math.c
//  C-Ray
//
//  Created by Valtteri Koskivuori on 28/12/2020.
//  Copyright © 2020-2022 Valtteri Koskivuori. All rights reserved.
//

#include <stdio.h>
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

static void dump_math_op(const enum mathOp op, char *buf, size_t bufsize) {
	switch(op) {
		case Add:
			snprintf(buf, bufsize, "add");
			break;
		case Subtract:
			snprintf(buf, bufsize, "subtract");
			break;
		case Multiply:
			snprintf(buf, bufsize, "multiply");
			break;
		case Divide:
			snprintf(buf, bufsize, "divide");
			break;
		case Power:
			snprintf(buf, bufsize, "power");
			break;
		case Log:
			snprintf(buf, bufsize, "log");
			break;
		case SquareRoot:
			snprintf(buf, bufsize, "sqrt");
			break;
		case InvSquareRoot:
			snprintf(buf, bufsize, "invsqrtf");
			break;
		case Absolute:
			snprintf(buf, bufsize, "abs");
			break;
		case Min:
			snprintf(buf, bufsize, "min");
			break;
		case Max:
			snprintf(buf, bufsize, "max");
			break;
		case LessThan:
			snprintf(buf, bufsize, "lt");
			break;
		case GreaterThan:
			snprintf(buf, bufsize, "gt");
			break;
		case Sign:
			snprintf(buf, bufsize, "sign");
			break;
		case Compare:
			snprintf(buf, bufsize, "compare");
			break;
		case Round:
			snprintf(buf, bufsize, "round");
			break;
		case Floor:
			snprintf(buf, bufsize, "floor");
			break;
		case Ceil:
			snprintf(buf, bufsize, "ceil");
			break;
		case Truncate:
			snprintf(buf, bufsize, "truncate");
			break;
		case Fraction:
			snprintf(buf, bufsize, "fraction");
			break;
		case Modulo:
			snprintf(buf, bufsize, "mod");
			break;
		case Sine:
			snprintf(buf, bufsize, "sin");
			break;
		case Cosine:
			snprintf(buf, bufsize, "cos");
			break;
		case Tangent:
			snprintf(buf, bufsize, "tan");
			break;
		case ToRadians:
			snprintf(buf, bufsize, "toradians");
			break;
		case ToDegrees:
			snprintf(buf, bufsize, "todegrees");
			break;
	}
}

static void dump(const void *node, char *dumpbuf, int bufsize) {
	struct mathNode *self = (struct mathNode *)node;
	char A[DUMPBUF_SIZE / 4] = "";
	char B[DUMPBUF_SIZE / 4] = "";
	char op[DUMPBUF_SIZE / 4] = "";
	if (self->A->base.dump) self->A->base.dump(self->A, A, sizeof(A));
	if (self->B->base.dump) self->B->base.dump(self->B, B, sizeof(B));
	dump_math_op(self->op, op, sizeof(op));
	snprintf(dumpbuf, bufsize, "mathNode { A: %s, B: %s, op: %s }", A, B, op);
}

float rough_compare(float a, float b) {
	if (fabsf(a - b) > 0.0000005) return false;
	return true;
}

static float eval(const struct valueNode *node, sampler *sampler, const struct hitRecord *record) {
	struct mathNode *this = (struct mathNode *)node;
	const float a = this->A->eval(this->A, sampler, record);
	const float b = this->B->eval(this->B, sampler, record);
	switch (this->op) {
		case Add:
			return a + b;
		case Subtract:
			return a - b;
		case Multiply:
			return a * b;
		case Divide:
			return a / b;
		case Power:
			return powf(a, b);
		case Log:
			return log10f(a);
		case SquareRoot:
			return sqrtf(a);
		case InvSquareRoot:
			return invsqrtf(a);
		case Absolute:
			return fabsf(a);
		case Min:
			return min(a, b);
		case Max:
			return max(a, b);
		case LessThan:
			return a < b ? 1.0f : 0.0f;
		case GreaterThan:
			return a > b ? 1.0f : 0.0f;
		case Sign:
			if (a > 0.0f) return 1.0f;
			if (a < 0.0f) return -1.0f;
			if (a == 0.0f) return 0.0f;
			break;
		case Compare:
			return rough_compare(a, b) ? 1.0f : 0.0f;
		case Round:
			return roundf(a);
		case Floor:
			return floorf(a);
		case Ceil:
			return ceilf(a);
		case Truncate:
			return truncf(a);
		case Fraction:
			return a - (int)a;
		case Modulo:
			return fmodf(a, b);
		case Sine:
			return sinf(a);
		case Cosine:
			return cosf(a);
		case Tangent:
			return tanf(a);
		case ToRadians:
			return toRadians(a);
		case ToDegrees:
			return fromRadians(a);
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
			.base = { .compare = compare, .dump = dump }
		}
	});
}
