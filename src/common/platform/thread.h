//
//  thread.h
//  C-ray
//
//  Created by Valtteri on 29.3.2020.
//  Copyright © 2020-2021 Valtteri Koskivuori. All rights reserved.
//

#pragma once

#ifdef WINDOWS
	#include <Windows.h>
#else
	#include <pthread.h>
#endif

#include "../dyn_array.h"

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

typedef struct cr_thread cr_thread;
dyn_array_def(cr_thread);

// Fetch the user data pointer from args parameter
void *thread_user_data(void *arg);

/// Start a new C-ray platform abstracted thread
/// @param t Pointer to the thread to be started
int thread_start(struct cr_thread *t);

/// Block until the given thread has terminated.
/// @param t Pointer to the thread to be checked.
void thread_wait(struct cr_thread *t);
