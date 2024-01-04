//
//  test_thread_pool.h
//  c-ray
//
//  Created by Valtteri on 04.1.2024.
//  Copyright © 2024 Valtteri Koskivuori. All rights reserved.
//

#include "../src/common/platform/thread_pool.h"
#include "../src/common/timer.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

void test_task(void *arg) {
	int *input = arg;
	int old = *input;

	*input += 1000;

	printf("tid=%lu, input=%d, val=%d\n", pthread_self(), old, *input);
	if (*input % 2) {
		timer_sleep_ms(10);
	}
}

bool test_thread_pool(void) {

	const size_t threads = 4;
	const int iterations = 100;

	struct cr_thread_pool *pool = thread_pool_create(threads);
	int *values = calloc(iterations, sizeof(*values));
	for (int i = 0; i < iterations; ++i) {
		values[i] = i;
		printf("values[i] = %i\n", i);
		thread_pool_enqueue(pool, test_task, values + i);
	}

	thread_pool_wait(pool);

	for (int i = 0; i < iterations; ++i) {
		test_assert(values[i] == i + 1000);
	}

	thread_pool_destroy(pool);
	free(values);

	return true;
}
