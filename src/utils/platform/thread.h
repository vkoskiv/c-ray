//
//  thread.h
//  C-ray
//
//  Created by Valtteri on 29.3.2020.
//  Copyright © 2020 Valtteri Koskivuori. All rights reserved.
//

#pragma once

#ifdef WINDOWS
	#include <Windows.h>
#else
	#include <pthread.h>
#endif

//Multi-platform threading

/**
 Thread information struct to communicate with main thread
 */
struct crThread {
#ifdef WINDOWS
	HANDLE thread_handle;
	DWORD thread_id;
#else
	pthread_t thread_id;
#endif
	void *userData; // Thread I/O.
	void *(*threadFunc)(void *); // Code you want to run.
};

// Fetch the user data pointer from args parameter
void *threadUserData(void *arg);

/// Start a new C-ray platform abstracted thread
/// @param t Pointer to the thread to be started
int threadStart(struct crThread *t);

/// Block until the given thread has terminated.
/// @param t Pointer to the thread to be checked.
void threadWait(struct crThread *t);
