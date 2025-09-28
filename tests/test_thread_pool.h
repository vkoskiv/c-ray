//
//  test_thread_pool.h
//  c-ray
//
//  Created by Valtteri on 04.1.2024.
//  Copyright © 2024 Valtteri Koskivuori. All rights reserved.
//

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <v.h>

// FIXME: Migrate to v.h test suite, once that's set up.

void test_task(void *arg) {
	int *input = arg;
	*input += 1000;
	if (*input % 2) {
		v_timer_sleep_ms(10);
	}
}

bool test_thread_pool(void) {

	const size_t threads = 8;
	const int iterations = 10;
	int loops = 100;

	v_threadpool *pool = v_threadpool_create(threads);
	int *values = calloc(iterations, sizeof(*values));
	while (loops--) {
		for (int i = 0; i < iterations; ++i) {
			values[i] = i;
			v_threadpool_enqueue(pool, test_task, &values[i]);
		}

		v_threadpool_wait(pool);

		for (int i = 0; i < iterations; ++i) {
			test_assert(values[i] == i + 1000);
		}
		memset(values, 0, iterations * sizeof(*values));
	}

	v_threadpool_destroy(pool);
	free(values);

	return true;
}
