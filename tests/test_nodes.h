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
#include "../src/nodes/converter/math.h"

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
	
	EXPECT_APPROXIMATE(result->eval(result, NULL), 0.0f);
	
	destroyScene(fake);
	return true;
}

bool mathnode_cosine(void) {
	struct world *fake = fakeWorld();
	const struct valueNode *A = newConstantValue(fake, M_PI);
	const struct valueNode *result = newMath(fake, A, NULL, Cosine);
	
	EXPECT_APPROXIMATE(result->eval(result, NULL), -1.0f);
	
	destroyScene(fake);
	return true;
}

bool mathnode_tangent(void) {
	struct world *fake = fakeWorld();
	const struct valueNode *A = newConstantValue(fake, M_PI);
	const struct valueNode *result = newMath(fake, A, NULL, Tangent);
	
	EXPECT_APPROXIMATE(result->eval(result, NULL), -0.0f);
	
	destroyScene(fake);
	return true;
}

bool mathnode_toradians(void) {
	struct world *fake = fakeWorld();
	const struct valueNode *A = newConstantValue(fake, 180.0f);
	const struct valueNode *result = newMath(fake, A, NULL, ToRadians);
	
	EXPECT_APPROXIMATE(result->eval(result, NULL), M_PI);
	
	destroyScene(fake);
	return true;
}

bool mathnode_todegrees(void) {
	struct world *fake = fakeWorld();
	const struct valueNode *A = newConstantValue(fake, M_PI);
	const struct valueNode *result = newMath(fake, A, NULL, ToDegrees);
	
	EXPECT_APPROXIMATE(result->eval(result, NULL), 180.0f);
	
	destroyScene(fake);
	return true;
}
