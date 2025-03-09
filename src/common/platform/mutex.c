//
//  mutex.c
//  c-ray
//
//  Created by Valtteri on 29.3.2020.
//  Copyright © 2020 Valtteri Koskivuori. All rights reserved.
//

#include "mutex.h"
#include <stdlib.h>

struct cr_mutex *mutex_create() {
	struct cr_mutex *new = calloc(1, sizeof(*new));
#ifdef WINDOWS
	InitializeCriticalSection(&new->lock);
#else
	new->lock = (pthread_mutex_t)PTHREAD_MUTEX_INITIALIZER;
#endif
	return new;
}

void mutex_destroy(struct cr_mutex *m) {
	if (!m) return;
#ifdef WINDOWS
	DeleteCriticalSection(m->lock);
#else
	pthread_mutex_destroy(&m->lock);
#endif
	free(m);
}

void mutex_lock(struct cr_mutex *m) {
#ifdef WINDOWS
	EnterCriticalSection(&m->lock);
#else
	pthread_mutex_lock(&m->lock);
#endif
}

void mutex_release(struct cr_mutex *m) {
#ifdef WINDOWS
	LeaveCriticalSection(&m->lock);
#else
	pthread_mutex_unlock(&m->lock);
#endif
}
