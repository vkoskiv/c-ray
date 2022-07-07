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

struct cr_mutex {
	#ifdef WINDOWS
		HANDLE tileMutex; // = INVALID_HANDLE_VALUE;
	#else
		pthread_mutex_t tileMutex; // = PTHREAD_MUTEX_INITIALIZER;
	#endif
};

struct cr_mutex *mutex_create() {
	struct cr_mutex *new = calloc(1, sizeof(*new));
#ifdef WINDOWS
	new->tileMutex = CreateMutex(NULL, FALSE, NULL);
#else
	new->tileMutex = (pthread_mutex_t)PTHREAD_MUTEX_INITIALIZER;
#endif
	return new;
}

void mutex_lock(struct cr_mutex *m) {
#ifdef WINDOWS
	WaitForSingleObject(m->tileMutex, INFINITE);
#else
	pthread_mutex_lock(&m->tileMutex);
#endif
}

void mutex_release(struct cr_mutex *m) {
#ifdef WINDOWS
	ReleaseMutex(m->tileMutex);
#else
	pthread_mutex_unlock(&m->tileMutex);
#endif
}
