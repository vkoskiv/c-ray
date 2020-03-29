//
//  mutex.c
//  C-ray
//
//  Created by Valtteri on 29.3.2020.
//  Copyright © 2020 Valtteri Koskivuori. All rights reserved.
//

#include "mutex.h"
#include <stdlib.h>

#ifdef WINDOWS
#include <Windows.h>
#else
#include <pthread.h>
#endif

struct crMutex {
	#ifdef WINDOWS
		HANDLE tileMutex; // = INVALID_HANDLE_VALUE;
	#else
		pthread_mutex_t tileMutex; // = PTHREAD_MUTEX_INITIALIZER;
	#endif
};

struct crMutex *createMutex() {
	struct crMutex *new = calloc(1, sizeof(struct crMutex));
#ifdef WINDOWS
	new->tileMutex = CreateMutex(NULL, FALSE, NULL);
#else
	new->tileMutex = (pthread_mutex_t)PTHREAD_MUTEX_INITIALIZER;
#endif
	return new;
}

void lockMutex(struct crMutex *m) {
#ifdef WINDOWS
	WaitForSingleObject(m->tileMutex, INFINITE);
#else
	pthread_mutex_lock(&m->tileMutex);
#endif
}

void releaseMutex(struct crMutex *m) {
#ifdef WINDOWS
	ReleaseMutex(m->tileMutex);
#else
	pthread_mutex_unlock(&m->tileMutex);
#endif
}
