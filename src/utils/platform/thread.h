//
//  thread.h
//  C-ray
//
//  Created by Valtteri on 29.3.2020.
//  Copyright © 2020 Valtteri Koskivuori. All rights reserved.
//

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

/// Spawn a new C-ray platform abstracted thread
/// @param t Pointer to the thread to be spawned
int startThread(struct crThread *t);

/// Block until the given thread has terminated.
/// @param t Pointer to the thread to be checked.
void checkThread(struct crThread *t);
