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
#include "../src/nodes/nodebase.h"
#include "../src/nodes/valuenode.h"
#include "../src/nodes/vectornode.h"
#include "../src/nodes/converter/math.h"
#include "../src/nodes/converter/map_range.h"

// The node system requires the world global memory pool and hash table for storage, so we fake one here
struct world *fakeWorld() {
	struct world *fake = calloc(1, sizeof(*fake));
	fake->nodePool = newBlock(NULL, 1024);
	fake->nodeTable = newHashtable(compareNodes, &fake->nodePool);
	return fake;
}

bool mathnode_add(void) {
	struct world *fake = fakeWorld();
	const struct valueNode *A = newConstantValue(fake, 128.0f);
	const struct valueNode *B = newConstantValue(fake, 128.0f);
	const struct valueNode *result = newMath(fake, A, B, Add);
	test_assert(result->eval(result, NULL) == 256.0f);
	
	A = newConstantValue(fake, -128.0f);
	B = newConstantValue(fake, 128.0f);
	result = newMath(fake, A, B, Add);
	test_assert(result->eval(result, NULL) == 0.0f);
	
	destroyScene(fake);
	return true;
}

bool mathnode_subtract(void) {
	struct world *fake = fakeWorld();
	const struct valueNode *A = newConstantValue(fake, 128.0f);
	const struct valueNode *B = newConstantValue(fake, 128.0f);
	const struct valueNode *result = newMath(fake, A, B, Subtract);
	test_assert(result->eval(result, NULL) == 0.0f);
	
	A = newConstantValue(fake, -128.0f);
	B = newConstantValue(fake, 128.0f);
	result = newMath(fake, A, B, Subtract);
	test_assert(result->eval(result, NULL) == -256.0f);
	
	destroyScene(fake);
	return true;
}

bool mathnode_multiply(void) {
	struct world *fake = fakeWorld();
	const struct valueNode *A = newConstantValue(fake, 128.0f);
	const struct valueNode *B = newConstantValue(fake, 128.0f);
	const struct valueNode *result = newMath(fake, A, B, Multiply);
	test_assert(result->eval(result, NULL) == 16384.0f);
	
	destroyScene(fake);
	return true;
}

bool mathnode_divide(void) {
	struct world *fake = fakeWorld();
	const struct valueNode *A = newConstantValue(fake, 128.0f);
	const struct valueNode *B = newConstantValue(fake, 128.0f);
	const struct valueNode *result = newMath(fake, A, B, Divide);
	test_assert(result->eval(result, NULL) == 1.0f);
	
	A = newConstantValue(fake, -128.0f);
	B = newConstantValue(fake, 1.0f);
	result = newMath(fake, A, B, Divide);
	test_assert(result->eval(result, NULL) == -128.0f);
	
	destroyScene(fake);
	return true;
}

bool mathnode_power(void) {
	struct world *fake = fakeWorld();
	const struct valueNode *A = newConstantValue(fake, 2.0f);
	const struct valueNode *B = newConstantValue(fake, 16.0f);
	const struct valueNode *result = newMath(fake, A, B, Power);
	test_assert(result->eval(result, NULL) == 65536.0f);
	
	A = newConstantValue(fake, -128.0f);
	B = newConstantValue(fake, 1.0f);
	result = newMath(fake, A, B, Power);
	test_assert(result->eval(result, NULL) == -128.0f);

	A = newConstantValue(fake, 128.0f);
	B = newConstantValue(fake, 0.0f);
	result = newMath(fake, A, B, Power);
	test_assert(result->eval(result, NULL) == 1.0f);
	
	destroyScene(fake);
	return true;
}

bool mathnode_log(void) {
	struct world *fake = fakeWorld();
	const struct valueNode *A = newConstantValue(fake, 1.0f);
	const struct valueNode *result = newMath(fake, A, NULL, Log);
	test_assert(result->eval(result, NULL) == 0.0f);
	
	A = newConstantValue(fake, 10.0f);
	result = newMath(fake, A, NULL , Log);
	test_assert(result->eval(result, NULL) == 1.0f);
	
	A = newConstantValue(fake, 100.0f);
	result = newMath(fake, A, NULL, Log);
	test_assert(result->eval(result, NULL) == 2.0f);
	
	A = newConstantValue(fake, 1000.0f);
	result = newMath(fake, A, NULL, Log);
	test_assert(result->eval(result, NULL) == 3.0f);
	
	A = newConstantValue(fake, 10000.0f);
	result = newMath(fake, A, NULL, Log);
	test_assert(result->eval(result, NULL) == 4.0f);
	
	destroyScene(fake);
	return true;
}

bool mathnode_squareroot(void) {
	struct world *fake = fakeWorld();
	const struct valueNode *A = newConstantValue(fake, 9.0f);
	const struct valueNode *result = newMath(fake, A, NULL, SquareRoot);
	test_assert(result->eval(result, NULL) == 3.0f);
	
	destroyScene(fake);
	return true;
}

bool mathnode_absolute(void) {
	struct world *fake = fakeWorld();
	const struct valueNode *A = newConstantValue(fake, -128.0f);
	const struct valueNode *result = newMath(fake, A, NULL, Absolute);
	test_assert(result->eval(result, NULL) == 128.0f);
	
	A = newConstantValue(fake, 128.0f);
	result = newMath(fake, A, NULL, Absolute);
	test_assert(result->eval(result, NULL) == 128.0f);
	
	destroyScene(fake);
	return true;
}

bool mathnode_min(void) {
	struct world *fake = fakeWorld();
	const struct valueNode *A = newConstantValue(fake, -128.0f);
	const struct valueNode *B = newConstantValue(fake, 128.0f);
	const struct valueNode *result = newMath(fake, A, B, Min);
	test_assert(result->eval(result, NULL) == -128.0f);
	
	A = newConstantValue(fake, 128.0f);
	B = newConstantValue(fake, 42.0f);
	result = newMath(fake, A, B, Min);
	test_assert(result->eval(result, NULL) == 42.0f);
	
	destroyScene(fake);
	return true;
}

bool mathnode_max(void) {
	struct world *fake = fakeWorld();
	const struct valueNode *A = newConstantValue(fake, -128.0f);
	const struct valueNode *B = newConstantValue(fake, 128.0f);
	const struct valueNode *result = newMath(fake, A, B, Max);
	test_assert(result->eval(result, NULL) == 128.0f);
	
	A = newConstantValue(fake, 128.0f);
	B = newConstantValue(fake, 42.0f);
	result = newMath(fake, A, B, Max);
	test_assert(result->eval(result, NULL) == 128.0f);
	
	destroyScene(fake);
	return true;
}

bool mathnode_sine(void) {
	struct world *fake = fakeWorld();
	const struct valueNode *A = newConstantValue(fake, M_PI);
	const struct valueNode *result = newMath(fake, A, NULL, Sine);
	
	roughly_equals(result->eval(result, NULL), 0.0f);
	
	destroyScene(fake);
	return true;
}

bool mathnode_cosine(void) {
	struct world *fake = fakeWorld();
	const struct valueNode *A = newConstantValue(fake, M_PI);
	const struct valueNode *result = newMath(fake, A, NULL, Cosine);
	
	roughly_equals(result->eval(result, NULL), -1.0f);
	
	destroyScene(fake);
	return true;
}

bool mathnode_tangent(void) {
	struct world *fake = fakeWorld();
	const struct valueNode *A = newConstantValue(fake, M_PI);
	const struct valueNode *result = newMath(fake, A, NULL, Tangent);
	
	roughly_equals(result->eval(result, NULL), -0.0f);
	
	destroyScene(fake);
	return true;
}

bool mathnode_toradians(void) {
	struct world *fake = fakeWorld();
	const struct valueNode *A = newConstantValue(fake, 180.0f);
	const struct valueNode *result = newMath(fake, A, NULL, ToRadians);
	
	roughly_equals(result->eval(result, NULL), M_PI);
	
	destroyScene(fake);
	return true;
}

bool mathnode_todegrees(void) {
	struct world *fake = fakeWorld();
	const struct valueNode *A = newConstantValue(fake, M_PI);
	const struct valueNode *result = newMath(fake, A, NULL, ToDegrees);
	
	roughly_equals(result->eval(result, NULL), 180.0f);
	
	destroyScene(fake);
	return true;
}

bool vecmath_vecAdd(void) {
	struct world *fake = fakeWorld();
	const struct vectorNode *A = newConstantVector(fake, (struct vector){1.0f, 2.0f, 3.0f});
	const struct vectorNode *B = newConstantVector(fake, (struct vector){1.0f, 2.0f, 3.0f});
	
	const struct vectorNode *add = newVecMath(fake, A, B, VecAdd);
	
	struct vectorValue result = add->eval(add, NULL);
	struct vector expected = (struct vector){2.0f, 4.0f, 6.0f};
	vec_roughly_equals(result.v, expected);
	
	destroyScene(fake);
	return true;
}

bool vecmath_vecSubtract(void) {
	struct world *fake = fakeWorld();
	const struct vectorNode *A = newConstantVector(fake, (struct vector){1.0f, 2.0f, 3.0f});
	const struct vectorNode *B = newConstantVector(fake, (struct vector){1.0f, 2.0f, 3.0f});
	
	const struct vectorNode *add = newVecMath(fake, A, B, VecSubtract);
	
	struct vectorValue result = add->eval(add, NULL);
	struct vector expected = vecZero();
	vec_roughly_equals(result.v, expected);
	
	destroyScene(fake);
	return true;
}

bool vecmath_vecMultiply(void) {
	struct world *fake = fakeWorld();
	const struct vectorNode *A = newConstantVector(fake, (struct vector){1.0f, 2.0f, 3.0f});
	const struct vectorNode *B = newConstantVector(fake, (struct vector){1.0f, 2.0f, 3.0f});
	
	const struct vectorNode *add = newVecMath(fake, A, B, VecMultiply);
	
	struct vectorValue result = add->eval(add, NULL);
	struct vector expected = (struct vector){1.0f, 4.0f, 9.0f};
	vec_roughly_equals(result.v, expected);
	
	destroyScene(fake);
	return true;
}

bool vecmath_vecAverage(void) {
	struct world *fake = fakeWorld();
	const struct vectorNode *A = newConstantVector(fake, (struct vector){0.0f, 0.0f, 0.0f});
	const struct vectorNode *B = newConstantVector(fake, (struct vector){5.0f, 5.0f, 5.0f});
	
	const struct vectorNode *op = newVecMath(fake, A, B, VecAverage);
	
	struct vectorValue result = op->eval(op, NULL);
	struct vector expected = (struct vector){2.5f, 2.5f, 2.5f};
	vec_roughly_equals(result.v, expected);
	
	destroyScene(fake);
	return true;
}
bool vecmath_vecDot(void) {
	struct world *fake = fakeWorld();
	const struct vectorNode *up = newConstantVector(fake, worldUp);
	const struct vectorNode *right = newConstantVector(fake, (struct vector){1.0f, 0.0f, 0.0f});
	
	const struct vectorNode *dot = newVecMath(fake, up, right, VecDot);
	
	struct vectorValue result = dot->eval(dot, NULL);
	roughly_equals(result.f, 0.0f);
	
	const struct vectorNode *down = newConstantVector(fake, vecNegate(worldUp));
	dot = newVecMath(fake, up, down, VecDot);
	result = dot->eval(dot, NULL);
	roughly_equals(result.f, -1.0f);
	
	dot = newVecMath(fake, up, up, VecDot);
	result = dot->eval(dot, NULL);
	roughly_equals(result.f, 1.0f);
	
	destroyScene(fake);
	return true;
}

bool vecmath_vecCross(void) {
	struct world *fake = fakeWorld();
	const struct vectorNode *A = newConstantVector(fake, (struct vector){1.0f, 0.0f, 0.0f});
	const struct vectorNode *B = newConstantVector(fake, (struct vector){0.0f, 1.0f, 0.0f});
	
	const struct vectorNode *op = newVecMath(fake, A, B, VecCross);
	
	struct vectorValue result = op->eval(op, NULL);
	struct vector expected = (struct vector){0.0f, 0.0f, 1.0f};
	vec_roughly_equals(result.v, expected);
	
	destroyScene(fake);
	return true;
}

bool vecmath_vecNormalize(void) {
	struct world *fake = fakeWorld();
	const struct vectorNode *A = newConstantVector(fake, (struct vector){1.0f, 2.0f, 3.0f});
	
	const struct vectorNode *op = newVecMath(fake, A, NULL, VecNormalize);
	
	float length = vecLength(op->eval(op, NULL).v);
	roughly_equals(length, 1.0f);
	
	destroyScene(fake);
	return true;
}

bool vecmath_vecReflect(void) {
	struct world *fake = fakeWorld();
	const struct vectorNode *toReflect = newConstantVector(fake, vecNormalize((struct vector){1.0f, 1.0f, 0.0f}));
	const struct vectorNode *normal = newConstantVector(fake, (struct vector){0.0f, -1.0f, 0.0f});
	
	const struct vectorNode *op = newVecMath(fake, toReflect, normal, VecReflect);
	
	struct vectorValue reflected = op->eval(op, NULL);
	roughly_equals(vecLength(reflected.v), 1.0f);
	
	struct vector expected = vecNormalize((struct vector){1.0f, -1.0f, 0.0f});
	vec_roughly_equals(reflected.v, expected);
	
	destroyScene(fake);
	return true;
}

bool vecmath_vecLength(void) {
	struct world *fake = fakeWorld();
	const struct vectorNode *A = newConstantVector(fake, (struct vector){0.0f, 2.0f, 0.0f});
	
	const struct vectorNode *op = newVecMath(fake, A, NULL, VecLength);
	
	struct vectorValue lengthValue = op->eval(op, NULL);
	roughly_equals(lengthValue.f, 2.0f);
	
	destroyScene(fake);
	return true;
}

bool vecmath_vecAbs(void) {
	struct world *fake = fakeWorld();
	const struct vectorNode *A = newConstantVector(fake, (struct vector){-10.0f, 2.0f, -3.0f});
	
	const struct vectorNode *op = newVecMath(fake, A, NULL, VecAbs);
	
	struct vectorValue result = op->eval(op, NULL);
	struct vector expected = (struct vector){10.0f, 2.0f, 3.0f};
	vec_roughly_equals(result.v, expected);
	
	destroyScene(fake);
	return true;
}

bool map_range(void) {
	struct world *fake = fakeWorld();
	
	const struct valueNode *zero = newConstantValue(fake, 0.0f);
	const struct valueNode *half = newConstantValue(fake, 0.5f);
	const struct valueNode *one = newConstantValue(fake, 1.0f);
	
	const struct valueNode *A = newMapRange(fake, half, zero, one, zero, one);
	test_assert(A->eval(A, NULL) == 0.5f);
	
	A = newMapRange(fake, half, zero, one, newConstantValue(fake, 0.0f), newConstantValue(fake, 30.0f));
	test_assert(A->eval(A, NULL) == 15.0f);
	
	A = newMapRange(fake, half, zero, one, newConstantValue(fake, -15.0f), newConstantValue(fake, 15.0f));
	test_assert(A->eval(A, NULL) == 0.0f);
	
	A = newMapRange(fake, newConstantValue(fake, 0.25f), zero, half, newConstantValue(fake, -15.0f), newConstantValue(fake, 15.0f));
	test_assert(A->eval(A, NULL) == 0.0f);
	
	A = newMapRange(fake, newConstantValue(fake, -1.0f), zero, one, zero, one);
	test_assert(A->eval(A, NULL) == 0.0f);
	
	A = newMapRange(fake, newConstantValue(fake, 2.0f), zero, one, zero, one);
	test_assert(A->eval(A, NULL) == 1.0f);
	
	A = newMapRange(fake, half, zero, one, zero, newConstantValue(fake, -5.0f));
	test_assert(A->eval(A, NULL) == -2.5f);
	
	A = newMapRange(fake, one, zero, one, zero, newConstantValue(fake, -5.0f));
	test_assert(A->eval(A, NULL) == -5.0f);
	
	A = newMapRange(fake, newConstantValue(fake, 2.5f), zero, newConstantValue(fake, 5.0f), zero, one);
	test_assert(A->eval(A, NULL) == 0.5f);
	
	A = newMapRange(fake, newConstantValue(fake, -2.5f), zero, newConstantValue(fake, -5.0f), zero, one);
	test_assert(A->eval(A, NULL) == 0.5f);
	
	destroyScene(fake);
	return true;
}
