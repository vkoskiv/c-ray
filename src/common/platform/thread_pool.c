//
//  thread_pool.c
//  c-ray
//
//  Created by Valtteri on 04.1.2024.
//  Copyright © 2024 Valtteri Koskivuori. All rights reserved.
//

#include <stdlib.h>
#include <v.h>

#include "thread_pool.h"
#include "signal.h"
#include "../logging.h"

// Mostly based on John Schember's excellent blog post:
// https://nachtimwald.com/2019/04/12/thread-pool-in-c/
//
// I just added an extra pool->first check to thread_pool_wait()
// since I discovered a race condition in my torture tests for this
// implementation. Basically, sometimes we could blow through a
// call to thread_pool_wait() if we enqueue a small amount of work
// and call it before threads had a chance to fetch work.

struct cr_task {
	void (*fn)(void *arg);
	void *arg;
	struct cr_task *next;
};

struct cr_thread_pool {
	struct cr_task *first;
	struct cr_task *last;
	struct v_mutex *mutex;
	v_cond *work_available;
	v_cond *work_ongoing;
	size_t active_threads;
	size_t alive_threads;
	bool stop_flag;
};

static struct cr_task *task_create(void (*fn)(void *arg), void *arg) {
	if (!fn) return NULL;
	struct cr_task *task = malloc(sizeof(*task));
	*task = (struct cr_task){
		.fn = fn,
		.arg = arg,
		.next = NULL
	};
	return task;
}

static struct cr_task *thread_pool_get_task(struct cr_thread_pool *pool) {
	if (!pool) return NULL;
	struct cr_task *task = pool->first;
	if (!task) return NULL;
	if (!task->next) {
		pool->first = NULL;
		pool->last = NULL;
	} else {
		pool->first = task->next;
	}
	return task;
}

static void *cr_worker(void *arg) {
	block_signals();
	struct cr_thread_pool *pool = arg;
	while (true) {
		v_mutex_lock(pool->mutex);
		while (!pool->first && !pool->stop_flag)
			v_cond_wait(pool->work_available, pool->mutex);
		if (pool->stop_flag) break;
		struct cr_task *task = thread_pool_get_task(pool);
		pool->active_threads++;
		logr(spam, "++threadpool => %zu\n", pool->active_threads);
		v_mutex_release(pool->mutex);
		if (task) {
			task->fn(task->arg);
			free(task);
		}
		v_mutex_lock(pool->mutex);
		pool->active_threads--;
		logr(spam, "--threadpool => %zu\n", pool->active_threads);
		if (!pool->stop_flag && pool->active_threads == 0 && !pool->first)
			v_cond_signal(pool->work_ongoing);
		v_mutex_release(pool->mutex);
	}
	pool->alive_threads--;
	v_cond_signal(pool->work_ongoing);
	v_mutex_release(pool->mutex);
	return NULL;
}

struct cr_thread_pool *thread_pool_create(size_t threads) {
	if (!threads)
		threads = 2;
	struct cr_thread_pool *pool = calloc(1, sizeof(*pool));
	logr(debug, "Spawning thread pool (%zut, %p)\n", threads, (void *)pool);
	pool->alive_threads = threads;

	pool->mutex = v_mutex_create();
	pool->work_available = v_cond_create();
	pool->work_ongoing = v_cond_create();

	v_thread_ctx ctx = {
		.thread_fn = cr_worker,
		.ctx = pool,
	};
	for (size_t i = 0; i < pool->alive_threads; ++i) {
		v_thread *ret = v_thread_create(ctx, v_thread_type_detached);
		if (!ret)
			goto fail;
	}
	return pool;
fail:
	v_mutex_destroy(pool->mutex);
	v_cond_destroy(pool->work_available);
	v_cond_destroy(pool->work_ongoing);
	free(pool);
	return NULL;
}

void thread_pool_destroy(struct cr_thread_pool *pool) {
	if (!pool)
		return;
	logr(debug, "Closing thread pool (%zut, %p)\n", pool->alive_threads, (void *)pool);
	v_mutex_lock(pool->mutex);
	// Clear work queue
	struct cr_task *head = pool->first;
	struct cr_task *next = NULL;
	while (head) {
		next = head->next;
		free(head);
		head = next;
	}
	// Tell the workers to stop
	pool->stop_flag = true;
	v_cond_broadcast(pool->work_available);
	v_mutex_release(pool->mutex);

	// Wait for them to actually stop
	thread_pool_wait(pool);

	v_mutex_destroy(pool->mutex);
	v_cond_destroy(pool->work_available);
	v_cond_destroy(pool->work_ongoing);
	free(pool);
}

bool thread_pool_enqueue(struct cr_thread_pool *pool, void (*fn)(void *arg), void *arg) {
	if (!pool) { // Fall back to running synchronously
		if (fn)
			fn(arg);
		return false;
	}
	struct cr_task *task = task_create(fn, arg);
	if (!task) return false;
	v_mutex_lock(pool->mutex);
	if (!pool->first) {
		pool->first = task;
		pool->last = pool->first;
	} else {
		pool->last->next = task;
		pool->last = task;
	}
	v_cond_broadcast(pool->work_available);
	v_mutex_release(pool->mutex);
	return true;
}

void thread_pool_wait(struct cr_thread_pool *pool) {
	if (!pool)
		return;
	v_mutex_lock(pool->mutex);
	while (true) {
		if (pool->first || (!pool->stop_flag && pool->active_threads != 0) || (pool->stop_flag && pool->alive_threads != 0)) {
			v_cond_wait(pool->work_ongoing, pool->mutex);
		} else {
			break;
		}
	}
	v_mutex_release(pool->mutex);
}
