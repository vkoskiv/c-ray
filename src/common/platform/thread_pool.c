//
//  thread_pool.c
//  c-ray
//
//  Created by Valtteri on 04.1.2024.
//  Copyright © 2024 Valtteri Koskivuori. All rights reserved.
//

#include "thread_pool.h"
#include "mutex.h"
#include "thread.h"
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
	struct cr_mutex *mutex;
	struct cr_cond work_available;
	struct cr_cond work_ongoing;
	size_t active_workers;
	struct cr_thread *threads;
	size_t thread_count;
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
	struct cr_thread_pool *pool = arg;
	while (true) {
		mutex_lock(pool->mutex);
		while (!pool->first && !pool->stop_flag)
			thread_cond_wait(&pool->work_available, pool->mutex);
		if (pool->stop_flag) break;
		struct cr_task *task = thread_pool_get_task(pool);
		pool->active_workers++;
		mutex_release(pool->mutex);
		if (task) {
			task->fn(task->arg);
			free(task);
		}
		mutex_lock(pool->mutex);
		pool->active_workers--;
		if (!pool->stop_flag && pool->active_workers == 0 && !pool->first)
			thread_cond_signal(&pool->work_ongoing);
		mutex_release(pool->mutex);
	}
	pool->thread_count--;
	thread_cond_signal(&pool->work_ongoing);
	mutex_release(pool->mutex);
	return NULL;
}

struct cr_thread_pool *thread_pool_create(size_t threads) {
	if (!threads) threads = 2;
	struct cr_thread_pool *pool = calloc(1, sizeof(*pool));
	logr(debug, "Spawning thread pool (%lut, %p)\n", threads, (void *)pool);
	pool->thread_count = threads;
	pool->threads = calloc(pool->thread_count, sizeof(*pool->threads));

	pool->mutex = mutex_create();
	thread_cond_init(&pool->work_available);
	thread_cond_init(&pool->work_ongoing);

	for (size_t i = 0; i < pool->thread_count; ++i) {
		pool->threads[i] = (struct cr_thread){
			.thread_fn = cr_worker,
			.user_data = pool
		};
		thread_create_detach(&pool->threads[i]);
	}
	return pool;
}

void thread_pool_destroy(struct cr_thread_pool *pool) {
	if (!pool) return;
	logr(debug, "Closing thread pool (%lut, %p)\n", pool->thread_count, (void *)pool);
	mutex_lock(pool->mutex);
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
	thread_cond_broadcast(&pool->work_available);
	mutex_release(pool->mutex);

	// Wait for them to actually stop
	thread_pool_wait(pool);

	mutex_destroy(pool->mutex);
	thread_cond_destroy(&pool->work_available);
	thread_cond_destroy(&pool->work_ongoing);
	free(pool->threads);
	free(pool);
}

bool thread_pool_enqueue(struct cr_thread_pool *pool, void (*fn)(void *arg), void *arg) {
	if (!pool) return false;
	struct cr_task *task = task_create(fn, arg);
	if (!task) return false;
	mutex_lock(pool->mutex);
	if (!pool->first) {
		pool->first = task;
		pool->last = pool->first;
	} else {
		pool->last->next = task;
		pool->last = task;
	}
	thread_cond_broadcast(&pool->work_available);
	mutex_release(pool->mutex);
	return true;
}

void thread_pool_wait(struct cr_thread_pool *pool) {
	if (!pool) return;
	mutex_lock(pool->mutex);
	while (true) {
		if (pool->first || (!pool->stop_flag && pool->active_workers != 0) || (pool->stop_flag && pool->thread_count != 0)) {
			thread_cond_wait(&pool->work_ongoing, pool->mutex);
		} else {
			break;
		}
	}
	mutex_release(pool->mutex);
}
