//
//  test_nodes.h
//  C-ray
//
//  Created by Valtteri on 26.6.2021.
//  Copyright © 2021 Valtteri Koskivuori. All rights reserved.
//

#pragma once

#include "../src/lib/datatypes/scene.h"
#include "../src/common/hashtable.h"
#include "../src/common/mempool.h"
#include "../src/lib/nodes/nodebase.h"
#include "../src/lib/nodes/valuenode.h"
#include "../src/lib/nodes/vectornode.h"
#include "../src/lib/nodes/converter/math.h"
#include "../src/lib/nodes/converter/map_range.h"
#include "../src/lib/renderer/samplers/sampler.h"

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
	struct sampler *sampler = newSampler();
	initSampler(sampler, Halton, 0, 16, 128);
	const struct valueNode *A = newConstantValue(s, 128.0f);
	const struct valueNode *B = newConstantValue(s, 128.0f);
	const struct valueNode *result = newMath(s, A, B, Add);
	test_assert(result->eval(result, sampler, NULL) == 256.0f);
	
	A = newConstantValue(s, -128.0f);
	B = newConstantValue(s, 128.0f);
	result = newMath(s, A, B, Add);
	test_assert(result->eval(result, sampler, NULL) == 0.0f);

	delete_storage(s);
	destroySampler(sampler);
	return true;
}

bool mathnode_subtract(void) {
	struct node_storage *s = make_storage();
	struct sampler *sampler = newSampler();
	initSampler(sampler, Halton, 0, 16, 128);
	const struct valueNode *A = newConstantValue(s, 128.0f);
	const struct valueNode *B = newConstantValue(s, 128.0f);
	const struct valueNode *result = newMath(s, A, B, Subtract);
	test_assert(result->eval(result, sampler, NULL) == 0.0f);
	
	A = newConstantValue(s, -128.0f);
	B = newConstantValue(s, 128.0f);
	result = newMath(s, A, B, Subtract);
	test_assert(result->eval(result, sampler, NULL) == -256.0f);
	
	delete_storage(s);
	destroySampler(sampler);
	return true;
}

bool mathnode_multiply(void) {
	struct node_storage *s = make_storage();
	struct sampler *sampler = newSampler();
	initSampler(sampler, Halton, 0, 16, 128);
	const struct valueNode *A = newConstantValue(s, 128.0f);
	const struct valueNode *B = newConstantValue(s, 128.0f);
	const struct valueNode *result = newMath(s, A, B, Multiply);
	test_assert(result->eval(result, sampler, NULL) == 16384.0f);
	
	delete_storage(s);
	destroySampler(sampler);
	return true;
}

bool mathnode_divide(void) {
	struct node_storage *s = make_storage();
	struct sampler *sampler = newSampler();
	initSampler(sampler, Halton, 0, 16, 128);
	const struct valueNode *A = newConstantValue(s, 128.0f);
	const struct valueNode *B = newConstantValue(s, 128.0f);
	const struct valueNode *result = newMath(s, A, B, Divide);
	test_assert(result->eval(result, sampler, NULL) == 1.0f);
	
	A = newConstantValue(s, -128.0f);
	B = newConstantValue(s, 1.0f);
	result = newMath(s, A, B, Divide);
	test_assert(result->eval(result, sampler, NULL) == -128.0f);
	
	delete_storage(s);
	destroySampler(sampler);
	return true;
}

bool mathnode_power(void) {
	struct node_storage *s = make_storage();
	struct sampler *sampler = newSampler();
	initSampler(sampler, Halton, 0, 16, 128);
	const struct valueNode *A = newConstantValue(s, 2.0f);
	const struct valueNode *B = newConstantValue(s, 16.0f);
	const struct valueNode *result = newMath(s, A, B, Power);
	test_assert(result->eval(result, sampler, NULL) == 65536.0f);
	
	A = newConstantValue(s, -128.0f);
	B = newConstantValue(s, 1.0f);
	result = newMath(s, A, B, Power);
	test_assert(result->eval(result, sampler, NULL) == -128.0f);

	A = newConstantValue(s, 128.0f);
	B = newConstantValue(s, 0.0f);
	result = newMath(s, A, B, Power);
	test_assert(result->eval(result, sampler, NULL) == 1.0f);
	
	delete_storage(s);
	destroySampler(sampler);
	return true;
}

bool mathnode_log(void) {
	struct node_storage *s = make_storage();
	struct sampler *sampler = newSampler();
	initSampler(sampler, Halton, 0, 16, 128);
	const struct valueNode *A = newConstantValue(s, 1.0f);
	const struct valueNode *result = newMath(s, A, NULL, Log);
	test_assert(result->eval(result, sampler, NULL) == 0.0f);
	
	A = newConstantValue(s, 10.0f);
	result = newMath(s, A, NULL , Log);
	test_assert(result->eval(result, sampler, NULL) == 1.0f);
	
	A = newConstantValue(s, 100.0f);
	result = newMath(s, A, NULL, Log);
	test_assert(result->eval(result, sampler, NULL) == 2.0f);
	
	A = newConstantValue(s, 1000.0f);
	result = newMath(s, A, NULL, Log);
	test_assert(result->eval(result, sampler, NULL) == 3.0f);
	
	A = newConstantValue(s, 10000.0f);
	result = newMath(s, A, NULL, Log);
	test_assert(result->eval(result, sampler, NULL) == 4.0f);
	
	delete_storage(s);
	destroySampler(sampler);
	return true;
}

bool mathnode_squareroot(void) {
	struct node_storage *s = make_storage();
	struct sampler *sampler = newSampler();
	initSampler(sampler, Halton, 0, 16, 128);
	const struct valueNode *A = newConstantValue(s, 9.0f);
	const struct valueNode *result = newMath(s, A, NULL, SquareRoot);
	test_assert(result->eval(result, sampler, NULL) == 3.0f);
	
	delete_storage(s);
	destroySampler(sampler);
	return true;
}

bool mathnode_invsquareroot(void) {
	struct node_storage *s = make_storage();
	struct sampler *sampler = newSampler();
	initSampler(sampler, Halton, 0, 16, 128);
	const struct valueNode *A = newConstantValue(s, 9.0f);
	const struct valueNode *result = newMath(s, A, NULL, InvSquareRoot);
	test_assert(result->eval(result, sampler, NULL) == 1.0f / 3.0f);
	
	delete_storage(s);
	destroySampler(sampler);
	return true;
}

bool mathnode_absolute(void) {
	struct node_storage *s = make_storage();
	struct sampler *sampler = newSampler();
	initSampler(sampler, Halton, 0, 16, 128);
	const struct valueNode *A = newConstantValue(s, -128.0f);
	const struct valueNode *result = newMath(s, A, NULL, Absolute);
	test_assert(result->eval(result, sampler, NULL) == 128.0f);
	
	A = newConstantValue(s, 128.0f);
	result = newMath(s, A, NULL, Absolute);
	test_assert(result->eval(result, sampler, NULL) == 128.0f);
	
	delete_storage(s);
	destroySampler(sampler);
	return true;
}

bool mathnode_min(void) {
	struct node_storage *s = make_storage();
	struct sampler *sampler = newSampler();
	initSampler(sampler, Halton, 0, 16, 128);
	const struct valueNode *A = newConstantValue(s, -128.0f);
	const struct valueNode *B = newConstantValue(s, 128.0f);
	const struct valueNode *result = newMath(s, A, B, Min);
	test_assert(result->eval(result, sampler, NULL) == -128.0f);
	
	A = newConstantValue(s, 128.0f);
	B = newConstantValue(s, 42.0f);
	result = newMath(s, A, B, Min);
	test_assert(result->eval(result, sampler, NULL) == 42.0f);
	
	delete_storage(s);
	destroySampler(sampler);
	return true;
}

bool mathnode_max(void) {
	struct node_storage *s = make_storage();
	struct sampler *sampler = newSampler();
	initSampler(sampler, Halton, 0, 16, 128);
	const struct valueNode *A = newConstantValue(s, -128.0f);
	const struct valueNode *B = newConstantValue(s, 128.0f);
	const struct valueNode *result = newMath(s, A, B, Max);
	test_assert(result->eval(result, sampler, NULL) == 128.0f);
	
	A = newConstantValue(s, 128.0f);
	B = newConstantValue(s, 42.0f);
	result = newMath(s, A, B, Max);
	test_assert(result->eval(result, sampler, NULL) == 128.0f);
	
	delete_storage(s);
	destroySampler(sampler);
	return true;
}

bool mathnode_lessthan(void) {
	struct node_storage *s = make_storage();
	struct sampler *sampler = newSampler();
	initSampler(sampler, Halton, 0, 16, 128);
	const struct valueNode *A = newConstantValue(s, 5.0f);
	const struct valueNode *B = newConstantValue(s, 10.0f);
	const struct valueNode *result = newMath(s, A, B, LessThan);
	test_assert(result->eval(result, sampler, NULL) == 1.0f);
	
	A = newConstantValue(s, 10.0f);
	B = newConstantValue(s, 10.0f);
	result = newMath(s, A, B, LessThan);
	test_assert(result->eval(result, sampler, NULL) == 0.0f);
	
	A = newConstantValue(s, 15.0f);
	B = newConstantValue(s, 10.0f);
	result = newMath(s, A, B, LessThan);
	test_assert(result->eval(result, sampler, NULL) == 0.0f);
	
	delete_storage(s);
	destroySampler(sampler);
	return true;
}

bool mathnode_greaterthan(void) {
	struct node_storage *s = make_storage();
	struct sampler *sampler = newSampler();
	initSampler(sampler, Halton, 0, 16, 128);
	const struct valueNode *A = newConstantValue(s, 5.0f);
	const struct valueNode *B = newConstantValue(s, 10.0f);
	const struct valueNode *result = newMath(s, A, B, GreaterThan);
	test_assert(result->eval(result, sampler, NULL) == 0.0f);
	
	A = newConstantValue(s, 10.0f);
	B = newConstantValue(s, 10.0f);
	result = newMath(s, A, B, GreaterThan);
	test_assert(result->eval(result, sampler, NULL) == 0.0f);
	
	A = newConstantValue(s, 15.0f);
	B = newConstantValue(s, 10.0f);
	result = newMath(s, A, B, GreaterThan);
	test_assert(result->eval(result, sampler, NULL) == 1.0f);
	
	delete_storage(s);
	destroySampler(sampler);
	return true;
}

bool mathnode_sign(void) {
	struct node_storage *s = make_storage();
	struct sampler *sampler = newSampler();
	initSampler(sampler, Halton, 0, 16, 128);
	const struct valueNode *A = newConstantValue(s, 5.0f);
	const struct valueNode *result = newMath(s, A, NULL, Sign);
	test_assert(result->eval(result, sampler, NULL) == 1.0f);
	
	A = newConstantValue(s, 10.0f);
	result = newMath(s, A, NULL, Sign);
	test_assert(result->eval(result, sampler, NULL) == 1.0f);
	
	A = newConstantValue(s, -5.0f);
	result = newMath(s, A, NULL, Sign);
	test_assert(result->eval(result, sampler, NULL) == -1.0f);
	
	A = newConstantValue(s, 0.0f);
	result = newMath(s, A, NULL, Sign);
	test_assert(result->eval(result, sampler, NULL) == 0.0f);
	
	delete_storage(s);
	destroySampler(sampler);
	return true;
}

bool mathnode_compare(void) {
	struct node_storage *s = make_storage();
	struct sampler *sampler = newSampler();
	initSampler(sampler, Halton, 0, 16, 128);
	const struct valueNode *A = newConstantValue(s, 5.0f);
	const struct valueNode *B = newConstantValue(s, 5.0f);
	const struct valueNode *result = newMath(s, A, B, Compare);
	test_assert(result->eval(result, sampler, NULL) == 1.0f);
	
	A = newConstantValue(s, 1.0f);
	B = newConstantValue(s, 1.0f);
	result = newMath(s, A, B, Compare);
	test_assert(result->eval(result, sampler, NULL) == 1.0f);
	
	A = newConstantValue(s, 1.0f);
	B = newConstantValue(s, 1.0000005f);
	result = newMath(s, A, B, Compare);
	test_assert(result->eval(result, sampler, NULL) == 1.0f);
	
	A = newConstantValue(s, 1.0f);
	B = newConstantValue(s, 1.0000005f);
	result = newMath(s, A, B, Compare);
	test_assert(result->eval(result, sampler, NULL) == 1.0f);
	
	A = newConstantValue(s, 1.0f);
	B = newConstantValue(s, 1.0000006f);
	result = newMath(s, A, B, Compare);
	test_assert(result->eval(result, sampler, NULL) == 0.0f);
	
	delete_storage(s);
	destroySampler(sampler);
	return true;
}

bool mathnode_round(void) {
	struct node_storage *s = make_storage();
	struct sampler *sampler = newSampler();
	initSampler(sampler, Halton, 0, 16, 128);
	const struct valueNode *A = newConstantValue(s, 0.0f);
	const struct valueNode *result = newMath(s, A, NULL, Round);
	test_assert(result->eval(result, sampler, NULL) == 0.0f);
	
	A = newConstantValue(s, 0.1f);
	result = newMath(s, A, NULL, Round);
	test_assert(result->eval(result, sampler, NULL) == 0.0f);
	
	A = newConstantValue(s, 0.2f);
	result = newMath(s, A, NULL, Round);
	test_assert(result->eval(result, sampler, NULL) == 0.0f);
	
	A = newConstantValue(s, 0.3f);
	result = newMath(s, A, NULL, Round);
	test_assert(result->eval(result, sampler, NULL) == 0.0f);
	
	A = newConstantValue(s, 0.4f);
	result = newMath(s, A, NULL, Round);
	test_assert(result->eval(result, sampler, NULL) == 0.0f);
	
	A = newConstantValue(s, 0.5f);
	result = newMath(s, A, NULL, Round);
	test_assert(result->eval(result, sampler, NULL) == 1.0f);
	
	A = newConstantValue(s, 0.6f);
	result = newMath(s, A, NULL, Round);
	test_assert(result->eval(result, sampler, NULL) == 1.0f);
	
	A = newConstantValue(s, 0.7f);
	result = newMath(s, A, NULL, Round);
	test_assert(result->eval(result, sampler, NULL) == 1.0f);
	
	A = newConstantValue(s, 0.8f);
	result = newMath(s, A, NULL, Round);
	test_assert(result->eval(result, sampler, NULL) == 1.0f);
	
	A = newConstantValue(s, 0.9f);
	result = newMath(s, A, NULL, Round);
	test_assert(result->eval(result, sampler, NULL) == 1.0f);
	
	A = newConstantValue(s, 1.0f);
	result = newMath(s, A, NULL, Round);
	test_assert(result->eval(result, sampler, NULL) == 1.0f);
	
	delete_storage(s);
	destroySampler(sampler);
	return true;
}

bool mathnode_floor(void) {
	struct node_storage *s = make_storage();
	struct sampler *sampler = newSampler();
	initSampler(sampler, Halton, 0, 16, 128);
	const struct valueNode *A = newConstantValue(s, 4.0f);
	const struct valueNode *result = newMath(s, A, NULL, Floor);
	test_assert(result->eval(result, sampler, NULL) == 4.0f);
	
	A = newConstantValue(s, 4.1f);
	result = newMath(s, A, NULL, Floor);
	test_assert(result->eval(result, sampler, NULL) == 4.0f);
	
	A = newConstantValue(s, 4.2f);
	result = newMath(s, A, NULL, Floor);
	test_assert(result->eval(result, sampler, NULL) == 4.0f);
	
	A = newConstantValue(s, 4.3f);
	result = newMath(s, A, NULL, Floor);
	test_assert(result->eval(result, sampler, NULL) == 4.0f);
	
	A = newConstantValue(s, 4.4f);
	result = newMath(s, A, NULL, Floor);
	test_assert(result->eval(result, sampler, NULL) == 4.0f);
	
	A = newConstantValue(s, 4.5f);
	result = newMath(s, A, NULL, Floor);
	test_assert(result->eval(result, sampler, NULL) == 4.0f);
	
	A = newConstantValue(s, 4.6f);
	result = newMath(s, A, NULL, Floor);
	test_assert(result->eval(result, sampler, NULL) == 4.0f);
	
	A = newConstantValue(s, 4.7f);
	result = newMath(s, A, NULL, Floor);
	test_assert(result->eval(result, sampler, NULL) == 4.0f);
	
	A = newConstantValue(s, 4.8f);
	result = newMath(s, A, NULL, Floor);
	test_assert(result->eval(result, sampler, NULL) == 4.0f);
	
	A = newConstantValue(s, 4.9f);
	result = newMath(s, A, NULL, Floor);
	test_assert(result->eval(result, sampler, NULL) == 4.0f);
	
	A = newConstantValue(s, 5.0f);
	result = newMath(s, A, NULL, Floor);
	test_assert(result->eval(result, sampler, NULL) == 5.0f);
	
	delete_storage(s);
	destroySampler(sampler);
	return true;
}

bool mathnode_ceil(void) {
	struct node_storage *s = make_storage();
	struct sampler *sampler = newSampler();
	initSampler(sampler, Halton, 0, 16, 128);
	const struct valueNode *A = newConstantValue(s, 4.0f);
	const struct valueNode *result = newMath(s, A, NULL, Ceil);
	test_assert(result->eval(result, sampler, NULL) == 4.0f);
	
	A = newConstantValue(s, 4.1f);
	result = newMath(s, A, NULL, Ceil);
	test_assert(result->eval(result, sampler, NULL) == 5.0f);
	
	A = newConstantValue(s, 4.2f);
	result = newMath(s, A, NULL, Ceil);
	test_assert(result->eval(result, sampler, NULL) == 5.0f);
	
	A = newConstantValue(s, 4.3f);
	result = newMath(s, A, NULL, Ceil);
	test_assert(result->eval(result, sampler, NULL) == 5.0f);
	
	A = newConstantValue(s, 4.4f);
	result = newMath(s, A, NULL, Ceil);
	test_assert(result->eval(result, sampler, NULL) == 5.0f);
	
	A = newConstantValue(s, 4.5f);
	result = newMath(s, A, NULL, Ceil);
	test_assert(result->eval(result, sampler, NULL) == 5.0f);
	
	A = newConstantValue(s, 4.6f);
	result = newMath(s, A, NULL, Ceil);
	test_assert(result->eval(result, sampler, NULL) == 5.0f);
	
	A = newConstantValue(s, 4.7f);
	result = newMath(s, A, NULL, Ceil);
	test_assert(result->eval(result, sampler, NULL) == 5.0f);
	
	A = newConstantValue(s, 4.8f);
	result = newMath(s, A, NULL, Ceil);
	test_assert(result->eval(result, sampler, NULL) == 5.0f);
	
	A = newConstantValue(s, 4.9f);
	result = newMath(s, A, NULL, Ceil);
	test_assert(result->eval(result, sampler, NULL) == 5.0f);
	
	A = newConstantValue(s, 5.0f);
	result = newMath(s, A, NULL, Ceil);
	test_assert(result->eval(result, sampler, NULL) == 5.0f);
	
	A = newConstantValue(s, 5.1f);
	result = newMath(s, A, NULL, Ceil);
	test_assert(result->eval(result, sampler, NULL) == 6.0f);
	
	delete_storage(s);
	destroySampler(sampler);
	return true;
}

bool mathnode_truncate(void) {
	struct node_storage *s = make_storage();
	struct sampler *sampler = newSampler();
	initSampler(sampler, Halton, 0, 16, 128);
	const struct valueNode *A = newConstantValue(s, 4.0f);
	const struct valueNode *result = newMath(s, A, NULL, Truncate);
	test_assert(result->eval(result, sampler, NULL) == 4.0f);
	
	A = newConstantValue(s, 4.1f);
	result = newMath(s, A, NULL, Truncate);
	test_assert(result->eval(result, sampler, NULL) == 4.0f);
	
	A = newConstantValue(s, 4.9f);
	result = newMath(s, A, NULL, Truncate);
	test_assert(result->eval(result, sampler, NULL) == 4.0f);
	
	A = newConstantValue(s, 4.3f);
	result = newMath(s, A, NULL, Truncate);
	test_assert(result->eval(result, sampler, NULL) == 4.0f);
	
	A = newConstantValue(s, 6.01f);
	result = newMath(s, A, NULL, Truncate);
	test_assert(result->eval(result, sampler, NULL) == 6.0f);
	
	A = newConstantValue(s, 4.5f);
	result = newMath(s, A, NULL, Truncate);
	test_assert(result->eval(result, sampler, NULL) == 4.0f);
	
	A = newConstantValue(s, 8.6f);
	result = newMath(s, A, NULL, Truncate);
	test_assert(result->eval(result, sampler, NULL) == 8.0f);
	
	A = newConstantValue(s, 9.7f);
	result = newMath(s, A, NULL, Truncate);
	test_assert(result->eval(result, sampler, NULL) == 9.0f);
	
	delete_storage(s);
	destroySampler(sampler);
	return true;
}

bool mathnode_fraction(void) {
	struct node_storage *s = make_storage();
	struct sampler *sampler = newSampler();
	initSampler(sampler, Halton, 0, 16, 128);
	const struct valueNode *A = newConstantValue(s, 4.0f);
	const struct valueNode *result = newMath(s, A, NULL, Fraction);
	test_assert(result->eval(result, sampler, NULL) == 0.0f);
	
	A = newConstantValue(s, 4.1f);
	result = newMath(s, A, NULL, Fraction);
	roughly_equals(result->eval(result, sampler, NULL), 0.1f);
	
	A = newConstantValue(s, 4.9f);
	result = newMath(s, A, NULL, Fraction);
	roughly_equals(result->eval(result, sampler, NULL), 0.9f);
	
	A = newConstantValue(s, 4.3f);
	result = newMath(s, A, NULL, Fraction);
	roughly_equals(result->eval(result, sampler, NULL), 0.3f);
	
	A = newConstantValue(s, 6.01f);
	result = newMath(s, A, NULL, Fraction);
	roughly_equals(result->eval(result, sampler, NULL), 0.01f);
	
	A = newConstantValue(s, 4.5f);
	result = newMath(s, A, NULL, Fraction);
	roughly_equals(result->eval(result, sampler, NULL), 0.5f);
	
	A = newConstantValue(s, 8.6f);
	result = newMath(s, A, NULL, Fraction);
	roughly_equals(result->eval(result, sampler, NULL), 0.6f);
	
	A = newConstantValue(s, 9.7f);
	result = newMath(s, A, NULL, Fraction);
	roughly_equals(result->eval(result, sampler, NULL), 0.7f);
	
	delete_storage(s);
	destroySampler(sampler);
	return true;
}

bool mathnode_modulo(void) {
	struct node_storage *s = make_storage();
	struct sampler *sampler = newSampler();
	initSampler(sampler, Halton, 0, 16, 128);
	const struct valueNode *A = newConstantValue(s, 4.0f);
	const struct valueNode *B = newConstantValue(s, 1.0f);
	const struct valueNode *result = newMath(s, A, B, Modulo);
	test_assert(result->eval(result, sampler, NULL) == 0.0f);
	
	A = newConstantValue(s, 4.0f);
	B = newConstantValue(s, 2.0f);
	result = newMath(s, A, B, Modulo);
	roughly_equals(result->eval(result, sampler, NULL), 0.0f);
	
	A = newConstantValue(s, 4.0f);
	B = newConstantValue(s, 3.0f);
	result = newMath(s, A, B, Modulo);
	roughly_equals(result->eval(result, sampler, NULL), 1.0f);
	
	A = newConstantValue(s, 4.0f);
	B = newConstantValue(s, 4.0f);
	result = newMath(s, A, B, Modulo);
	roughly_equals(result->eval(result, sampler, NULL), 0.0f);
	
	A = newConstantValue(s, 4.0f);
	B = newConstantValue(s, 5.0f);
	result = newMath(s, A, B, Modulo);
	roughly_equals(result->eval(result, sampler, NULL), 4.0f);
	
	delete_storage(s);
	destroySampler(sampler);
	return true;
}

bool mathnode_sine(void) {
	struct node_storage *s = make_storage();
	struct sampler *sampler = newSampler();
	initSampler(sampler, Halton, 0, 16, 128);
	const struct valueNode *A = newConstantValue(s, M_PI);
	const struct valueNode *result = newMath(s, A, NULL, Sine);
	
	roughly_equals(result->eval(result, sampler, NULL), 0.0f);
	
	delete_storage(s);
	destroySampler(sampler);
	return true;
}

bool mathnode_cosine(void) {
	struct node_storage *s = make_storage();
	struct sampler *sampler = newSampler();
	initSampler(sampler, Halton, 0, 16, 128);
	const struct valueNode *A = newConstantValue(s, M_PI);
	const struct valueNode *result = newMath(s, A, NULL, Cosine);
	
	roughly_equals(result->eval(result, sampler, NULL), -1.0f);
	
	delete_storage(s);
	destroySampler(sampler);
	return true;
}

bool mathnode_tangent(void) {
	struct node_storage *s = make_storage();
	struct sampler *sampler = newSampler();
	initSampler(sampler, Halton, 0, 16, 128);
	const struct valueNode *A = newConstantValue(s, M_PI);
	const struct valueNode *result = newMath(s, A, NULL, Tangent);
	
	roughly_equals(result->eval(result, sampler, NULL), -0.0f);
	
	delete_storage(s);
	destroySampler(sampler);
	return true;
}

bool mathnode_toradians(void) {
	struct node_storage *s = make_storage();
	struct sampler *sampler = newSampler();
	initSampler(sampler, Halton, 0, 16, 128);
	const struct valueNode *A = newConstantValue(s, 180.0f);
	const struct valueNode *result = newMath(s, A, NULL, ToRadians);
	
	roughly_equals(result->eval(result, sampler, NULL), M_PI);
	
	delete_storage(s);
	destroySampler(sampler);
	return true;
}

bool mathnode_todegrees(void) {
	struct node_storage *s = make_storage();
	struct sampler *sampler = newSampler();
	initSampler(sampler, Halton, 0, 16, 128);
	const struct valueNode *A = newConstantValue(s, M_PI);
	const struct valueNode *result = newMath(s, A, NULL, ToDegrees);
	
	roughly_equals(result->eval(result, sampler, NULL), 180.0f);
	
	delete_storage(s);
	destroySampler(sampler);
	return true;
}

bool vecmath_vecAdd(void) {
	struct node_storage *s = make_storage();
	struct sampler *sampler = newSampler();
	initSampler(sampler, Halton, 0, 16, 128);
	const struct vectorNode *A = newConstantVector(s, (struct vector){1.0f, 2.0f, 3.0f});
	const struct vectorNode *B = newConstantVector(s, (struct vector){1.0f, 2.0f, 3.0f});
	
	const struct vectorNode *add = newVecMath(s, A, B, NULL, NULL, VecAdd);
	
	struct vectorValue result = add->eval(add, sampler, NULL);
	struct vector expected = (struct vector){2.0f, 4.0f, 6.0f};
	vec_roughly_equals(result.v, expected);
	
	delete_storage(s);
	destroySampler(sampler);
	return true;
}

bool vecmath_vecSubtract(void) {
	struct node_storage *s = make_storage();
	struct sampler *sampler = newSampler();
	initSampler(sampler, Halton, 0, 16, 128);
	const struct vectorNode *A = newConstantVector(s, (struct vector){1.0f, 2.0f, 3.0f});
	const struct vectorNode *B = newConstantVector(s, (struct vector){1.0f, 2.0f, 3.0f});
	
	const struct vectorNode *add = newVecMath(s, A, B, NULL, NULL, VecSubtract);
	
	struct vectorValue result = add->eval(add, sampler, NULL);
	struct vector expected = vec_zero();
	vec_roughly_equals(result.v, expected);
	
	delete_storage(s);
	destroySampler(sampler);
	return true;
}

bool vecmath_vecMultiply(void) {
	struct node_storage *s = make_storage();
	struct sampler *sampler = newSampler();
	initSampler(sampler, Halton, 0, 16, 128);
	const struct vectorNode *A = newConstantVector(s, (struct vector){1.0f, 2.0f, 3.0f});
	const struct vectorNode *B = newConstantVector(s, (struct vector){1.0f, 2.0f, 3.0f});
	
	const struct vectorNode *add = newVecMath(s, A, B, NULL, NULL, VecMultiply);
	
	struct vectorValue result = add->eval(add, sampler, NULL);
	struct vector expected = (struct vector){1.0f, 4.0f, 9.0f};
	vec_roughly_equals(result.v, expected);
	
	delete_storage(s);
	destroySampler(sampler);
	return true;
}

bool vecmath_vecDot(void) {
	struct node_storage *s = make_storage();
	struct sampler *sampler = newSampler();
	initSampler(sampler, Halton, 0, 16, 128);
	const struct vectorNode *up = newConstantVector(s, g_world_up);
	const struct vectorNode *right = newConstantVector(s, (struct vector){1.0f, 0.0f, 0.0f});
	
	const struct vectorNode *dot = newVecMath(s, up, right, NULL, NULL, VecDot);
	
	struct vectorValue result = dot->eval(dot, sampler, NULL);
	roughly_equals(result.f, 0.0f);
	
	const struct vectorNode *down = newConstantVector(s, vec_negate(g_world_up));
	dot = newVecMath(s, up, down, NULL, NULL, VecDot);
	result = dot->eval(dot, sampler, NULL);
	roughly_equals(result.f, -1.0f);
	
	dot = newVecMath(s, up, up, NULL, NULL, VecDot);
	result = dot->eval(dot, sampler, NULL);
	roughly_equals(result.f, 1.0f);
	
	delete_storage(s);
	destroySampler(sampler);
	return true;
}

bool vecmath_vecCross(void) {
	struct node_storage *s = make_storage();
	struct sampler *sampler = newSampler();
	initSampler(sampler, Halton, 0, 16, 128);
	const struct vectorNode *A = newConstantVector(s, (struct vector){1.0f, 0.0f, 0.0f});
	const struct vectorNode *B = newConstantVector(s, (struct vector){0.0f, 1.0f, 0.0f});
	
	const struct vectorNode *op = newVecMath(s, A, B, NULL, NULL, VecCross);
	
	struct vectorValue result = op->eval(op, sampler, NULL);
	struct vector expected = (struct vector){0.0f, 0.0f, 1.0f};
	vec_roughly_equals(result.v, expected);
	
	delete_storage(s);
	destroySampler(sampler);
	return true;
}

bool vecmath_vecNormalize(void) {
	struct node_storage *s = make_storage();
	struct sampler *sampler = newSampler();
	initSampler(sampler, Halton, 0, 16, 128);
	const struct vectorNode *A = newConstantVector(s, (struct vector){1.0f, 2.0f, 3.0f});
	
	const struct vectorNode *op = newVecMath(s, A, NULL, NULL, NULL, VecNormalize);
	
	float length = vec_length(op->eval(op, sampler, NULL).v);
	roughly_equals(length, 1.0f);
	
	delete_storage(s);
	destroySampler(sampler);
	return true;
}

bool vecmath_vecReflect(void) {
	struct node_storage *s = make_storage();
	struct sampler *sampler = newSampler();
	initSampler(sampler, Halton, 0, 16, 128);
	const struct vectorNode *toReflect = newConstantVector(s, vec_normalize((struct vector){1.0f, 1.0f, 0.0f}));
	const struct vectorNode *normal = newConstantVector(s, (struct vector){0.0f, -1.0f, 0.0f});
	
	const struct vectorNode *op = newVecMath(s, toReflect, normal, NULL, NULL, VecReflect);
	
	struct vectorValue reflected = op->eval(op, sampler, NULL);
	roughly_equals(vec_length(reflected.v), 1.0f);
	
	struct vector expected = vec_normalize((struct vector){1.0f, -1.0f, 0.0f});
	vec_roughly_equals(reflected.v, expected);
	
	delete_storage(s);
	destroySampler(sampler);
	return true;
}

bool vecmath_vecLength(void) {
	struct sampler *sampler = newSampler();
	initSampler(sampler, Halton, 0, 16, 128);
	struct node_storage *s = make_storage();
	const struct vectorNode *A = newConstantVector(s, (struct vector){0.0f, 2.0f, 0.0f});
	
	const struct vectorNode *op = newVecMath(s, A, NULL, NULL, NULL, VecLength);
	
	struct vectorValue lengthValue = op->eval(op, sampler, NULL);
	roughly_equals(lengthValue.f, 2.0f);
	
	delete_storage(s);
	destroySampler(sampler);
	return true;
}

bool vecmath_vecAbs(void) {
	struct node_storage *s = make_storage();
	struct sampler *sampler = newSampler();
	initSampler(sampler, Halton, 0, 16, 128);
	const struct vectorNode *A = newConstantVector(s, (struct vector){-10.0f, 2.0f, -3.0f});
	
	const struct vectorNode *op = newVecMath(s, A, NULL, NULL, NULL, VecAbs);
	
	struct vectorValue result = op->eval(op, sampler, NULL);
	struct vector expected = (struct vector){10.0f, 2.0f, 3.0f};
	vec_roughly_equals(result.v, expected);
	
	delete_storage(s);
	destroySampler(sampler);
	return true;
}

bool vecmath_vecScale(void) {
	struct node_storage *s = make_storage();
	struct sampler *sampler = newSampler();
	initSampler(sampler, Halton, 0, 16, 128);
	const struct vectorNode *A = newConstantVector(s, (struct vector){ 1.0f, 0.0f, 0.0f });
	
	const struct vectorNode *op = newVecMath(s, A, NULL, NULL, newConstantValue(s, 2.0f), VecScale);
	
	struct vectorValue result = op->eval(op, sampler, NULL);
	struct vector expected = (struct vector){ 2.0f, 0.0f, 0.0f };
	vec_roughly_equals(result.v, expected);
	
	delete_storage(s);
	destroySampler(sampler);
	return true;
}

bool map_range(void) {
	struct node_storage *s = make_storage();
	struct sampler *sampler = newSampler();
	initSampler(sampler, Halton, 0, 16, 128);
	
	const struct valueNode *zero = newConstantValue(s, 0.0f);
	const struct valueNode *half = newConstantValue(s, 0.5f);
	const struct valueNode *one = newConstantValue(s, 1.0f);
	
	const struct valueNode *A = newMapRange(s, half, zero, one, zero, one);
	test_assert(A->eval(A, sampler, NULL) == 0.5f);
	
	A = newMapRange(s, half, zero, one, newConstantValue(s, 0.0f), newConstantValue(s, 30.0f));
	test_assert(A->eval(A, sampler, NULL) == 15.0f);
	
	A = newMapRange(s, half, zero, one, newConstantValue(s, -15.0f), newConstantValue(s, 15.0f));
	test_assert(A->eval(A, sampler, NULL) == 0.0f);
	
	A = newMapRange(s, newConstantValue(s, 0.25f), zero, half, newConstantValue(s, -15.0f), newConstantValue(s, 15.0f));
	test_assert(A->eval(A, sampler, NULL) == 0.0f);
	
	A = newMapRange(s, newConstantValue(s, -1.0f), zero, one, zero, one);
	test_assert(A->eval(A, sampler, NULL) == 0.0f);
	
	A = newMapRange(s, newConstantValue(s, 2.0f), zero, one, zero, one);
	test_assert(A->eval(A, sampler, NULL) == 1.0f);
	
	A = newMapRange(s, half, zero, one, zero, newConstantValue(s, -5.0f));
	test_assert(A->eval(A, sampler, NULL) == -2.5f);
	
	A = newMapRange(s, one, zero, one, zero, newConstantValue(s, -5.0f));
	test_assert(A->eval(A, sampler, NULL) == -5.0f);
	
	A = newMapRange(s, newConstantValue(s, 2.5f), zero, newConstantValue(s, 5.0f), zero, one);
	test_assert(A->eval(A, sampler, NULL) == 0.5f);
	
	A = newMapRange(s, newConstantValue(s, -2.5f), zero, newConstantValue(s, -5.0f), zero, one);
	test_assert(A->eval(A, sampler, NULL) == 0.5f);
	
	delete_storage(s);
	destroySampler(sampler);
	return true;
}
