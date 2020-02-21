//
//  multiplatform.h
//  C-Ray
//
//  Created by Valtteri on 19/03/2019.
//  Copyright © 2015-2020 Valtteri Koskivuori. All rights reserved.
//

#include <stdbool.h>
#include <stdint.h>

//Multi-platform stubs for thread-handling and other operations that differ on *nix and WIN32

///Prepare terminal. On *nix this disables output buffering, on WIN32 it enables ANSI escape codes.
void initTerminal(void);

/// Restore previous terminal state. On *nix it un-hides the cursor.
void restoreTerminal(void);

/// Get amount of logical processing cores on the system
/// @remark Is unaware of NUMA nodes on high core count systems
/// @return Amount of logical processing cores
int getSysCores(void);

//Multi-platform mutex

struct crMutex {
	#ifdef WINDOWS
		HANDLE tileMutex; // = INVALID_HANDLE_VALUE;
	#else
		pthread_mutex_t tileMutex; // = PTHREAD_MUTEX_INITIALIZER;
	#endif
};

struct crMutex *createMutex(void);

void lockMutex(struct crMutex *m);

void releaseMutex(struct crMutex *m);

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
	int thread_num;
	bool threadComplete;
	
	bool paused; //SDL listens for P key pressed, which sets these, one for each thread.
	
	//Share info about the current tile with main thread
	int currentTileNum;
	int completedSamples;
	
	uint64_t totalSamples;
	
	long avgSampleTime; //Single tile pass
	
	struct renderer *r;
	struct texture *output;
	
	void *(*threadFunc)(void *);
};


/// Block until the given thread has terminated.
/// @param t Pointer to the thread to be checked.
void checkThread(struct crThread *t);

/// Spawn a new C-ray platform abstracted thread
/// @param t Pointer to the thread to be spawned
int spawnThread(struct crThread *t);


//void
