//
//  test_nodes.h
//  C-ray
//
//  Created by Valtteri on 26.6.2021.
//  Copyright © 2021 Valtteri Koskivuori. All rights reserved.
//

#pragma once

#include "../src/datatypes/scene.h"
#include "../src/utils/hashtable.h"
#include "../src/utils/mempool.h"
#include "../src/nodes/nodebase.h"
#include "../src/nodes/valuenode.h"
#include "../src/nodes/vectornode.h"
#include "../src/nodes/converter/math.h"
#include "../src/nodes/converter/map_range.h"

struct node_storage *make_storage() {
	struct node_storage *storage = calloc(1, sizeof(*storage));
	storage->node_pool = newBlock(NULL, 1024);
	storage->node_table = newHashtable(compareNodes, &storage->node_pool);
	return storage;
}

void delete_storage(struct node_storage *storage) {
	destroyHashtable(storage->node_table);
	destroyBlocks(storage->node_pool);
	free(storage);
}

bool mathnode_add(void) {
	struct node_storage *s = make_storage();
	const struct valueNode *A = newConstantValue(s, 128.0f);
	const struct valueNode *B = newConstantValue(s, 128.0f);
	const struct valueNode *result = newMath(s, A, B, Add);
	test_assert(result->eval(result, NULL) == 256.0f);
	
	A = newConstantValue(s, -128.0f);
	B = newConstantValue(s, 128.0f);
	result = newMath(s, A, B, Add);
	test_assert(result->eval(result, NULL) == 0.0f);

	delete_storage(s);
	return true;
}

bool mathnode_subtract(void) {
	struct node_storage *s = make_storage();
	const struct valueNode *A = newConstantValue(s, 128.0f);
	const struct valueNode *B = newConstantValue(s, 128.0f);
	const struct valueNode *result = newMath(s, A, B, Subtract);
	test_assert(result->eval(result, NULL) == 0.0f);
	
	A = newConstantValue(s, -128.0f);
	B = newConstantValue(s, 128.0f);
	result = newMath(s, A, B, Subtract);
	test_assert(result->eval(result, NULL) == -256.0f);
	
	delete_storage(s);
	return true;
}

bool mathnode_multiply(void) {
	struct node_storage *s = make_storage();
	const struct valueNode *A = newConstantValue(s, 128.0f);
	const struct valueNode *B = newConstantValue(s, 128.0f);
	const struct valueNode *result = newMath(s, A, B, Multiply);
	test_assert(result->eval(result, NULL) == 16384.0f);
	
	delete_storage(s);
	return true;
}

bool mathnode_divide(void) {
	struct node_storage *s = make_storage();
	const struct valueNode *A = newConstantValue(s, 128.0f);
	const struct valueNode *B = newConstantValue(s, 128.0f);
	const struct valueNode *result = newMath(s, A, B, Divide);
	test_assert(result->eval(result, NULL) == 1.0f);
	
	A = newConstantValue(s, -128.0f);
	B = newConstantValue(s, 1.0f);
	result = newMath(s, A, B, Divide);
	test_assert(result->eval(result, NULL) == -128.0f);
	
	delete_storage(s);
	return true;
}

bool mathnode_power(void) {
	struct node_storage *s = make_storage();
	const struct valueNode *A = newConstantValue(s, 2.0f);
	const struct valueNode *B = newConstantValue(s, 16.0f);
	const struct valueNode *result = newMath(s, A, B, Power);
	test_assert(result->eval(result, NULL) == 65536.0f);
	
	A = newConstantValue(s, -128.0f);
	B = newConstantValue(s, 1.0f);
	result = newMath(s, A, B, Power);
	test_assert(result->eval(result, NULL) == -128.0f);

	A = newConstantValue(s, 128.0f);
	B = newConstantValue(s, 0.0f);
	result = newMath(s, A, B, Power);
	test_assert(result->eval(result, NULL) == 1.0f);
	
	delete_storage(s);
	return true;
}

bool mathnode_log(void) {
	struct node_storage *s = make_storage();
	const struct valueNode *A = newConstantValue(s, 1.0f);
	const struct valueNode *result = newMath(s, A, NULL, Log);
	test_assert(result->eval(result, NULL) == 0.0f);
	
	A = newConstantValue(s, 10.0f);
	result = newMath(s, A, NULL , Log);
	test_assert(result->eval(result, NULL) == 1.0f);
	
	A = newConstantValue(s, 100.0f);
	result = newMath(s, A, NULL, Log);
	test_assert(result->eval(result, NULL) == 2.0f);
	
	A = newConstantValue(s, 1000.0f);
	result = newMath(s, A, NULL, Log);
	test_assert(result->eval(result, NULL) == 3.0f);
	
	A = newConstantValue(s, 10000.0f);
	result = newMath(s, A, NULL, Log);
	test_assert(result->eval(result, NULL) == 4.0f);
	
	delete_storage(s);
	return true;
}

bool mathnode_squareroot(void) {
	struct node_storage *s = make_storage();
	const struct valueNode *A = newConstantValue(s, 9.0f);
	const struct valueNode *result = newMath(s, A, NULL, SquareRoot);
	test_assert(result->eval(result, NULL) == 3.0f);
	
	delete_storage(s);
	return true;
}

bool mathnode_absolute(void) {
	struct node_storage *s = make_storage();
	const struct valueNode *A = newConstantValue(s, -128.0f);
	const struct valueNode *result = newMath(s, A, NULL, Absolute);
	test_assert(result->eval(result, NULL) == 128.0f);
	
	A = newConstantValue(s, 128.0f);
	result = newMath(s, A, NULL, Absolute);
	test_assert(result->eval(result, NULL) == 128.0f);
	
	delete_storage(s);
	return true;
}

bool mathnode_min(void) {
	struct node_storage *s = make_storage();
	const struct valueNode *A = newConstantValue(s, -128.0f);
	const struct valueNode *B = newConstantValue(s, 128.0f);
	const struct valueNode *result = newMath(s, A, B, Min);
	test_assert(result->eval(result, NULL) == -128.0f);
	
	A = newConstantValue(s, 128.0f);
	B = newConstantValue(s, 42.0f);
	result = newMath(s, A, B, Min);
	test_assert(result->eval(result, NULL) == 42.0f);
	
	delete_storage(s);
	return true;
}

bool mathnode_max(void) {
	struct node_storage *s = make_storage();
	const struct valueNode *A = newConstantValue(s, -128.0f);
	const struct valueNode *B = newConstantValue(s, 128.0f);
	const struct valueNode *result = newMath(s, A, B, Max);
	test_assert(result->eval(result, NULL) == 128.0f);
	
	A = newConstantValue(s, 128.0f);
	B = newConstantValue(s, 42.0f);
	result = newMath(s, A, B, Max);
	test_assert(result->eval(result, NULL) == 128.0f);
	
	delete_storage(s);
	return true;
}

bool mathnode_sine(void) {
	struct node_storage *s = make_storage();
	const struct valueNode *A = newConstantValue(s, M_PI);
	const struct valueNode *result = newMath(s, A, NULL, Sine);
	
	roughly_equals(result->eval(result, NULL), 0.0f);
	
	delete_storage(s);
	return true;
}

bool mathnode_cosine(void) {
	struct node_storage *s = make_storage();
	const struct valueNode *A = newConstantValue(s, M_PI);
	const struct valueNode *result = newMath(s, A, NULL, Cosine);
	
	roughly_equals(result->eval(result, NULL), -1.0f);
	
	delete_storage(s);
	return true;
}

bool mathnode_tangent(void) {
	struct node_storage *s = make_storage();
	const struct valueNode *A = newConstantValue(s, M_PI);
	const struct valueNode *result = newMath(s, A, NULL, Tangent);
	
	roughly_equals(result->eval(result, NULL), -0.0f);
	
	delete_storage(s);
	return true;
}

bool mathnode_toradians(void) {
	struct node_storage *s = make_storage();
	const struct valueNode *A = newConstantValue(s, 180.0f);
	const struct valueNode *result = newMath(s, A, NULL, ToRadians);
	
	roughly_equals(result->eval(result, NULL), M_PI);
	
	delete_storage(s);
	return true;
}

bool mathnode_todegrees(void) {
	struct node_storage *s = make_storage();
	const struct valueNode *A = newConstantValue(s, M_PI);
	const struct valueNode *result = newMath(s, A, NULL, ToDegrees);
	
	roughly_equals(result->eval(result, NULL), 180.0f);
	
	delete_storage(s);
	return true;
}

bool vecmath_vecAdd(void) {
	struct node_storage *s = make_storage();
	const struct vectorNode *A = newConstantVector(s, (struct vector){1.0f, 2.0f, 3.0f});
	const struct vectorNode *B = newConstantVector(s, (struct vector){1.0f, 2.0f, 3.0f});
	
	const struct vectorNode *add = newVecMath(s, A, B, VecAdd);
	
	struct vectorValue result = add->eval(add, NULL);
	struct vector expected = (struct vector){2.0f, 4.0f, 6.0f};
	vec_roughly_equals(result.v, expected);
	
	delete_storage(s);
	return true;
}

bool vecmath_vecSubtract(void) {
	struct node_storage *s = make_storage();
	const struct vectorNode *A = newConstantVector(s, (struct vector){1.0f, 2.0f, 3.0f});
	const struct vectorNode *B = newConstantVector(s, (struct vector){1.0f, 2.0f, 3.0f});
	
	const struct vectorNode *add = newVecMath(s, A, B, VecSubtract);
	
	struct vectorValue result = add->eval(add, NULL);
	struct vector expected = vecZero();
	vec_roughly_equals(result.v, expected);
	
	delete_storage(s);
	return true;
}

bool vecmath_vecMultiply(void) {
	struct node_storage *s = make_storage();
	const struct vectorNode *A = newConstantVector(s, (struct vector){1.0f, 2.0f, 3.0f});
	const struct vectorNode *B = newConstantVector(s, (struct vector){1.0f, 2.0f, 3.0f});
	
	const struct vectorNode *add = newVecMath(s, A, B, VecMultiply);
	
	struct vectorValue result = add->eval(add, NULL);
	struct vector expected = (struct vector){1.0f, 4.0f, 9.0f};
	vec_roughly_equals(result.v, expected);
	
	delete_storage(s);
	return true;
}

bool vecmath_vecAverage(void) {
	struct node_storage *s = make_storage();
	const struct vectorNode *A = newConstantVector(s, (struct vector){0.0f, 0.0f, 0.0f});
	const struct vectorNode *B = newConstantVector(s, (struct vector){5.0f, 5.0f, 5.0f});
	
	const struct vectorNode *op = newVecMath(s, A, B, VecAverage);
	
	struct vectorValue result = op->eval(op, NULL);
	struct vector expected = (struct vector){2.5f, 2.5f, 2.5f};
	vec_roughly_equals(result.v, expected);
	
	delete_storage(s);
	return true;
}
bool vecmath_vecDot(void) {
	struct node_storage *s = make_storage();
	const struct vectorNode *up = newConstantVector(s, worldUp);
	const struct vectorNode *right = newConstantVector(s, (struct vector){1.0f, 0.0f, 0.0f});
	
	const struct vectorNode *dot = newVecMath(s, up, right, VecDot);
	
	struct vectorValue result = dot->eval(dot, NULL);
	roughly_equals(result.f, 0.0f);
	
	const struct vectorNode *down = newConstantVector(s, vecNegate(worldUp));
	dot = newVecMath(s, up, down, VecDot);
	result = dot->eval(dot, NULL);
	roughly_equals(result.f, -1.0f);
	
	dot = newVecMath(s, up, up, VecDot);
	result = dot->eval(dot, NULL);
	roughly_equals(result.f, 1.0f);
	
	delete_storage(s);
	return true;
}

bool vecmath_vecCross(void) {
	struct node_storage *s = make_storage();
	const struct vectorNode *A = newConstantVector(s, (struct vector){1.0f, 0.0f, 0.0f});
	const struct vectorNode *B = newConstantVector(s, (struct vector){0.0f, 1.0f, 0.0f});
	
	const struct vectorNode *op = newVecMath(s, A, B, VecCross);
	
	struct vectorValue result = op->eval(op, NULL);
	struct vector expected = (struct vector){0.0f, 0.0f, 1.0f};
	vec_roughly_equals(result.v, expected);
	
	delete_storage(s);
	return true;
}

bool vecmath_vecNormalize(void) {
	struct node_storage *s = make_storage();
	const struct vectorNode *A = newConstantVector(s, (struct vector){1.0f, 2.0f, 3.0f});
	
	const struct vectorNode *op = newVecMath(s, A, NULL, VecNormalize);
	
	float length = vecLength(op->eval(op, NULL).v);
	roughly_equals(length, 1.0f);
	
	delete_storage(s);
	return true;
}

bool vecmath_vecReflect(void) {
	struct node_storage *s = make_storage();
	const struct vectorNode *toReflect = newConstantVector(s, vecNormalize((struct vector){1.0f, 1.0f, 0.0f}));
	const struct vectorNode *normal = newConstantVector(s, (struct vector){0.0f, -1.0f, 0.0f});
	
	const struct vectorNode *op = newVecMath(s, toReflect, normal, VecReflect);
	
	struct vectorValue reflected = op->eval(op, NULL);
	roughly_equals(vecLength(reflected.v), 1.0f);
	
	struct vector expected = vecNormalize((struct vector){1.0f, -1.0f, 0.0f});
	vec_roughly_equals(reflected.v, expected);
	
	delete_storage(s);
	return true;
}

bool vecmath_vecLength(void) {
	struct node_storage *s = make_storage();
	const struct vectorNode *A = newConstantVector(s, (struct vector){0.0f, 2.0f, 0.0f});
	
	const struct vectorNode *op = newVecMath(s, A, NULL, VecLength);
	
	struct vectorValue lengthValue = op->eval(op, NULL);
	roughly_equals(lengthValue.f, 2.0f);
	
	delete_storage(s);
	return true;
}

bool vecmath_vecAbs(void) {
	struct node_storage *s = make_storage();
	const struct vectorNode *A = newConstantVector(s, (struct vector){-10.0f, 2.0f, -3.0f});
	
	const struct vectorNode *op = newVecMath(s, A, NULL, VecAbs);
	
	struct vectorValue result = op->eval(op, NULL);
	struct vector expected = (struct vector){10.0f, 2.0f, 3.0f};
	vec_roughly_equals(result.v, expected);
	
	delete_storage(s);
	return true;
}

bool map_range(void) {
	struct node_storage *s = make_storage();
	
	const struct valueNode *zero = newConstantValue(s, 0.0f);
	const struct valueNode *half = newConstantValue(s, 0.5f);
	const struct valueNode *one = newConstantValue(s, 1.0f);
	
	const struct valueNode *A = newMapRange(s, half, zero, one, zero, one);
	test_assert(A->eval(A, NULL) == 0.5f);
	
	A = newMapRange(s, half, zero, one, newConstantValue(s, 0.0f), newConstantValue(s, 30.0f));
	test_assert(A->eval(A, NULL) == 15.0f);
	
	A = newMapRange(s, half, zero, one, newConstantValue(s, -15.0f), newConstantValue(s, 15.0f));
	test_assert(A->eval(A, NULL) == 0.0f);
	
	A = newMapRange(s, newConstantValue(s, 0.25f), zero, half, newConstantValue(s, -15.0f), newConstantValue(s, 15.0f));
	test_assert(A->eval(A, NULL) == 0.0f);
	
	A = newMapRange(s, newConstantValue(s, -1.0f), zero, one, zero, one);
	test_assert(A->eval(A, NULL) == 0.0f);
	
	A = newMapRange(s, newConstantValue(s, 2.0f), zero, one, zero, one);
	test_assert(A->eval(A, NULL) == 1.0f);
	
	A = newMapRange(s, half, zero, one, zero, newConstantValue(s, -5.0f));
	test_assert(A->eval(A, NULL) == -2.5f);
	
	A = newMapRange(s, one, zero, one, zero, newConstantValue(s, -5.0f));
	test_assert(A->eval(A, NULL) == -5.0f);
	
	A = newMapRange(s, newConstantValue(s, 2.5f), zero, newConstantValue(s, 5.0f), zero, one);
	test_assert(A->eval(A, NULL) == 0.5f);
	
	A = newMapRange(s, newConstantValue(s, -2.5f), zero, newConstantValue(s, -5.0f), zero, one);
	test_assert(A->eval(A, NULL) == 0.5f);
	
	delete_storage(s);
	return true;
}
