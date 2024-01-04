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

//Multi-platform threading

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

typedef struct cr_thread cr_thread;
dyn_array_def(cr_thread);

// Fetch the user data pointer from args parameter
void *thread_user_data(void *arg);

// Create & detach a thread. Used by thread pool.
int thread_create_detach(struct cr_thread *t);
/// Start a new c-ray platform abstracted thread
/// @param t Pointer to the thread to be started
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
