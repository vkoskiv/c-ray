//
//  dyn_array.h
//  c-ray
//
//  Created by Valtteri Koskivuori on 05/11/2023
//  Copyright © 2023 Valtteri Koskivuori. All rights reserved.
//

#ifndef _DYN_ARRAY_H_
#define _DYN_ARRAY_H_

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#ifndef DYN_ARR_START_SIZE
#define DYN_ARR_START_SIZE 16
#endif

static inline size_t grow_x_1_5(size_t capacity, size_t elem_size) {
	if (capacity == 0)
		return DYN_ARR_START_SIZE;
	if (capacity > SIZE_MAX / (size_t)(1.5 * elem_size))
		return 0;
	return capacity + (capacity / 2);
}

static inline size_t grow_x_2(size_t capacity, size_t elem_size) {
	if (capacity == 0)
		return DYN_ARR_START_SIZE;
	if (capacity > SIZE_MAX / (2 * elem_size))
		return 0;
	return capacity * 2;
}

#define dyn_array_def(T) \
	size_t T##_arr_add(struct T##_arr *a, T value) { \
		if (a->count >= a->capacity) { \
			size_t new_capacity = a->grow_fn ? a->grow_fn(a->capacity, sizeof(*a->items)) : grow_x_2(a->capacity, sizeof(*a->items)); \
			a->items = realloc(a->items, sizeof(*a->items) * new_capacity); \
			a->capacity = new_capacity; \
		} \
		a->items[a->count] = value; \
		return a->count++; \
	} \
	void T##_arr_trim(struct T##_arr *a) { \
		if (!a || a->count >= a->capacity) return; \
		T *new = malloc(a->count * sizeof(*a)); \
		memcpy(new, a->items, a->count * sizeof(*a)); \
		free(a->items); \
		a->items = new; \
		a->capacity = a->count; \
	} \
	void T##_arr_free(struct T##_arr *a) { \
		if (!a) return; \
		if (a->items) free(a->items); \
		a->items = NULL; \
		a->capacity = 0; \
		a->count = 0; \
	}

#define dyn_array_dec(T) \
	struct T##_arr { \
		T *items; \
		size_t count; \
		size_t capacity; \
		size_t (*grow_fn)(size_t capacity, size_t item_size); \
	}; \
	size_t T##_arr_add(struct T##_arr *a, T value); \
	void T##_arr_trim(struct T##_arr *a); \
	void T##_arr_free(struct T##_arr *a);

dyn_array_dec(int);
dyn_array_dec(float);
dyn_array_dec(size_t);

#endif
