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
#include "../includes.h"

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
	struct T##_arr { \
		T *items; \
		size_t count; \
		size_t capacity; \
		size_t (*grow_fn)(size_t capacity, size_t item_size); \
		void   (*elem_free)(T *item); \
	}; \
	static inline size_t CR_UNUSED T##_arr_add(struct T##_arr *a, const T value) { \
		if (a->count >= a->capacity) { \
			size_t new_capacity = a->grow_fn ? a->grow_fn(a->capacity, sizeof(*a->items)) : grow_x_2(a->capacity, sizeof(*a->items)); \
			a->items = realloc(a->items, sizeof(*a->items) * new_capacity); \
			a->capacity = new_capacity; \
		} \
		a->items[a->count] = value; \
		return a->count++; \
	} \
	static inline void CR_UNUSED T##_arr_trim(struct T##_arr *a) { \
		if (!a || a->count >= a->capacity) return; \
		T *new = malloc(a->count * sizeof(*a)); \
		memcpy(new, a->items, a->count * sizeof(*a)); \
		free(a->items); \
		a->items = new; \
		a->capacity = a->count; \
	} \
	static inline struct T##_arr CR_UNUSED T##_arr_copy(const struct T##_arr a) { \
		if (!a.items) return (struct T##_arr){ 0 }; \
		struct T##_arr c = a; \
		c.items = malloc(a.count * sizeof(*a.items)); \
		memcpy(c.items, a.items, a.count * sizeof(*a.items)); \
		return c; \
	} \
	static inline void CR_UNUSED T##_arr_free(struct T##_arr *a) { \
		if (!a) return; \
		if (a->elem_free) { \
			for (size_t i = 0; i < a->count; ++i) \
				a->elem_free(&a->items[i]);\
		}\
		if (a->items) free(a->items); \
		a->items = NULL; \
		a->capacity = 0; \
		a->count = 0; \
	} \
	static inline void CR_UNUSED T##_arr_join(struct T##_arr *a, struct T##_arr *b) { \
		if (!a || !b) return; \
		for (size_t i = 0; i < b->count; ++i) \
			T##_arr_add(a, b->items[i]); \
		T##_arr_free(b); \
	}

dyn_array_def(int);
dyn_array_def(float);
dyn_array_def(size_t);

#endif
