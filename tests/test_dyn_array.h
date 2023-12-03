//
//  test_arraylist.h
//  c-ray
//
//  Created by Valtteri on 05/11/2023
//  Copyright © 2023 Valtteri Koskivuori. All rights reserved.
//

#pragma once

#include "../src/common/dyn_array.h"
#include "../src/common/vector.h"

#define dyn_test_count 1000

bool dyn_array_basic(void) {
	struct int_arr arr = { 0 };
	test_assert(arr.capacity == 0);
	test_assert(arr.count == 0);

	for (int i = 0; i < dyn_test_count; ++i) {
		int_arr_add(&arr, i);
	}

	test_assert(arr.count == dyn_test_count);
	for (int i = 0; i < dyn_test_count; ++i) {
		test_assert(arr.items[i] == i);
	}

	int_arr_free(&arr);

	test_assert(arr.count == 0);
	test_assert(arr.capacity == 0);
	test_assert(!arr.items);

	return true;
}

bool dyn_array_linear_grow(void) {
	struct int_arr arr = { 0 };
	arr.grow_fn = grow_x_1_5;
	test_assert(arr.capacity == 0);
	test_assert(arr.count == 0);

	for (int i = 0; i < dyn_test_count; ++i) {
		int_arr_add(&arr, i);
	}

	test_assert(arr.count == dyn_test_count);
	for (int i = 0; i < dyn_test_count; ++i) {
		test_assert(arr.items[i] == i);
	}

	int_arr_free(&arr);

	test_assert(arr.count == 0);
	test_assert(arr.capacity == 0);
	test_assert(!arr.items);

	return true;
}

struct foo {
	int a;
	float b;
	char c;
};
typedef struct foo foo;
dyn_array_def(foo);

bool dyn_array_custom(void) {
	struct foo_arr arr = { 0 };
	test_assert(arr.capacity == 0);
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

	test_assert(arr.count == 0);
	test_assert(arr.capacity == 0);
	test_assert(!arr.items);

	return true;
}

bool dyn_array_trim(void) {
	struct int_arr arr = { 0 };
	test_assert(arr.capacity == 0);
	test_assert(arr.count == 0);

	for (int i = 0; i < dyn_test_count; ++i) {
		int_arr_add(&arr, i);
	}

	test_assert(arr.count == dyn_test_count);

	for (int i = 0; i < dyn_test_count; ++i) {
		test_assert(arr.items[i] == i);
	}

	test_assert(arr.count == 1000);
	test_assert(arr.capacity == 1024);

	int_arr_trim(&arr);

	test_assert(arr.count == 1000);
	test_assert(arr.capacity == 1000);

	for (int i = 0; i < dyn_test_count; ++i) {
		test_assert(arr.items[i] == i);
	}

	arr.count = 500;
	int_arr_trim(&arr);
	test_assert(arr.count == 500);
	test_assert(arr.capacity == 500);

	for (int i = 0; i < 500; ++i) {
		test_assert(arr.items[i] == i);
	}

	int_arr_free(&arr);

	test_assert(arr.count == 0);
	test_assert(arr.capacity == 0);
	test_assert(!arr.items);

	return true;
}

bool dyn_array_trim_expand(void) {
	struct int_arr arr = { 0 };
	test_assert(arr.capacity == 0);
	test_assert(arr.count == 0);

	for (int i = 0; i < dyn_test_count; ++i) {
		int_arr_add(&arr, i);
	}

	test_assert(arr.count == dyn_test_count);

	for (int i = 0; i < dyn_test_count; ++i) {
		test_assert(arr.items[i] == i);
	}

	test_assert(arr.count == 1000);
	test_assert(arr.capacity == 1024);

	int_arr_trim(&arr);

	test_assert(arr.count == 1000);
	test_assert(arr.capacity == 1000);

	for (int i = 0; i < dyn_test_count; ++i) {
		test_assert(arr.items[i] == i);
	}

	arr.count = 500;
	int_arr_trim(&arr);
	test_assert(arr.count == 500);
	test_assert(arr.capacity == 500);

	for (int i = 0; i < 500; ++i) {
		test_assert(arr.items[i] == i);
	}
	
	for(int i = 0; i < 500; ++i) {
		int_arr_add(&arr, i);
	}

	test_assert(arr.count == 1000);

	for (int i = 0; i < 500; ++i) {
		test_assert(arr.items[i] == i);
	}
	for (int i = 0; i < 500; ++i) {
		test_assert(arr.items[i + 500] == i);
	}

	int_arr_free(&arr);

	test_assert(arr.count == 0);
	test_assert(arr.capacity == 0);
	test_assert(!arr.items);

	return true;
}

bool dyn_array_copy(void) {
	struct int_arr arr = { 0 };

	for (int i = 0; i < dyn_test_count; ++i) {
		int_arr_add(&arr, i);
	}

	struct int_arr no_copy = arr;
	test_assert(no_copy.count == arr.count);
	test_assert(no_copy.capacity == arr.capacity);
	test_assert(no_copy.elem_free == arr.elem_free);
	test_assert(no_copy.grow_fn == arr.grow_fn);
	test_assert(no_copy.items == arr.items);

	struct int_arr copy = int_arr_copy(arr);
	test_assert(copy.count == arr.count);
	test_assert(copy.capacity == arr.capacity);
	test_assert(copy.elem_free == arr.elem_free);
	test_assert(copy.grow_fn == arr.grow_fn);
	test_assert(copy.items != arr.items);

	int_arr_free(&arr);
	for (int i = 0; i < dyn_test_count; ++i) {
		test_assert(copy.items[i] == i);
	}
	int_arr_free(&copy);

	return true;
}

bool dyn_array_join(void) {
	struct int_arr a = { 0 };
	for (int i = 0; i < dyn_test_count / 2; ++i) {
		int_arr_add(&a, i);
	}

	struct int_arr b = { 0 };
	for (int i = 0; i < dyn_test_count / 2; ++i) {
		int_arr_add(&b, i + (dyn_test_count / 2));
	}

	test_assert(a.count == dyn_test_count / 2);
	test_assert(b.count == dyn_test_count / 2);

	struct int_arr combined = { 0 };
	int_arr_join(&combined, &a);
	test_assert(combined.count == dyn_test_count / 2);
	test_assert(a.count == 0);
	test_assert(a.capacity == 0);
	test_assert(!a.items);
	for (int i = 0; i < dyn_test_count / 2; ++i) {
		test_assert(combined.items[i] == i);
	}

	int_arr_join(&combined, &b);
	test_assert(combined.count == dyn_test_count);
	test_assert(b.count == 0);
	test_assert(b.capacity == 0);
	test_assert(!b.items);
	for (int i = 0; i < dyn_test_count; ++i) {
		test_assert(combined.items[i] == i);
	}

	return true;
}
