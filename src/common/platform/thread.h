//
//  thread.h
//  c-ray
//
//  Created by Valtteri on 29.3.2020.
//  Copyright © 2020-2024 Valtteri Koskivuori. All rights reserved.
//

#pragma once

#ifdef WINDOWS
	#include <stdbool.h>
	#include <Windows.h>
#else
	#include <pthread.h>
#endif

#include "../dyn_array.h"
#include "mutex.h"

// small thread/sync abstraction for POSIX and Windows

// #define CR_SIMULATE_NOTHREADS

/**
 Thread information struct to communicate with main thread
 */
struct cr_thread {
#ifdef WINDOWS
	HANDLE thread_handle;
	DWORD thread_id;
#else
	pthread_t thread_id;
#endif
	void *user_data; // Thread I/O.
	void *(*thread_fn)(void *); // Code you want to run.
};

struct cr_cond {
#ifdef WINDOWS
	PCONDITION_VARIABLE cond;
#else
	pthread_cond_t cond;
#endif
};

struct cr_rwlock {
#ifdef WINDOWS
	SRWLOCK lock;
	bool exclusive;
#else
	pthread_rwlock_t lock;
#endif
};

typedef struct cr_thread cr_thread;
dyn_array_def(cr_thread)

// Create & detach a thread. Used by thread pool.
int thread_create_detach(struct cr_thread *t);

// NOTE: t may have automatic lifetime, but t->user_data contents may not.
int thread_start(struct cr_thread *t);

/// Block until the given thread has terminated.
/// @param t Pointer to the thread to be checked.
void thread_wait(struct cr_thread *t);

int thread_cond_init(struct cr_cond *cond);

int thread_cond_destroy(struct cr_cond *cond);

int thread_cond_wait(struct cr_cond *cond, struct cr_mutex *mutex);

void ms_to_timespec(struct timespec *ts, unsigned int ms);

int thread_cond_timed_wait(struct cr_cond *cond, struct cr_mutex *mutex, const struct timespec *absolute_time);

int thread_cond_signal(struct cr_cond *cond);

int thread_cond_broadcast(struct cr_cond *cond);

int thread_rwlock_init(struct cr_rwlock *lock);
int thread_rwlock_destroy(struct cr_rwlock *lock);
int thread_rwlock_rdlock(struct cr_rwlock *lock);
int thread_rwlock_wrlock(struct cr_rwlock *lock);
int thread_rwlock_unlock(struct cr_rwlock *lock);
