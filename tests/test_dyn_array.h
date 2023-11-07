//
//  test_arraylist.h
//  c-ray
//
//  Created by Valtteri on 05/11/2023
//  Copyright © 2023 Valtteri Koskivuori. All rights reserved.
//

#pragma once

#define DYN_ARRAY_IMPL
#include "../src/utils/dyn_array.h"
#include "../src/utils/logging.h"

#define dyn_test_count 1000

bool dyn_array_basic(void) {
	struct int_arr arr = { 0 };
	test_assert(arr.remaining == 0);
	test_assert(arr.count == 0);

	for (int i = 0; i < dyn_test_count; ++i) {
		int_arr_add(&arr, i);
	}

	test_assert(arr.count == dyn_test_count);
	for (int i = 0; i < dyn_test_count; ++i) {
		test_assert(arr.items[i] == i);
	}

	int_arr_free(&arr);
	return true;
}

bool dyn_array_linear(void) {
	struct int_arr arr = { 0 };
	arr.grow_fn = grow_x_1_5;
	test_assert(arr.remaining == 0);
	test_assert(arr.count == 0);

	for (int i = 0; i < dyn_test_count; ++i) {
		int_arr_add(&arr, i);
	}


	test_assert(arr.count == dyn_test_count);
	for (int i = 0; i < dyn_test_count; ++i) {
		test_assert(arr.items[i] == i);
	}

	int_arr_free(&arr);
	return true;
}

struct foo {
	int a;
	float b;
	char c;
};
typedef struct foo foo;
dyn_array_dec(foo);
dyn_array_def(foo);

bool dyn_array_custom(void) {
	struct foo_arr arr = { 0 };
	test_assert(arr.remaining == 0);
	test_assert(arr.count == 0);

	for (int i = 0; i < dyn_test_count; ++i) {
		foo_arr_add(&arr, (struct foo){ .a = i, .b = 0.5f * i, .c = 'v' });
	}

	test_assert(arr.count == dyn_test_count);

	for (int i = 0; i < dyn_test_count; ++i) {
		struct foo e = arr.items[i];
		test_assert(e.a == i);
		test_assert(e.b == i * 0.5f);
		test_assert(e.c == 'v');
	}

	foo_arr_free(&arr);

	return true;
}
