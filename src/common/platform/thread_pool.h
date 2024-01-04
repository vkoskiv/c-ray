//
//  thread_pool.h
//  c-ray
//
//  Created by Valtteri on 04.1.2024.
//  Copyright © 2024 Valtteri Koskivuori. All rights reserved.
//

#include <stddef.h>
#include <stdbool.h>

struct cr_thread_pool;

struct cr_thread_pool *thread_pool_create(size_t threads);
void thread_pool_destroy(struct cr_thread_pool *pool);

bool thread_pool_enqueue(struct cr_thread_pool *pool, void (*fn)(void *arg), void *arg);
void thread_pool_wait(struct cr_thread_pool *pool);
