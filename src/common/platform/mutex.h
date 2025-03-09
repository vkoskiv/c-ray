//
//  mutex.h
//  c-ray
//
//  Created by Valtteri on 29.3.2020.
//  Copyright © 2020 Valtteri Koskivuori. All rights reserved.
//

#pragma once

//Platform-agnostic mutexes

#ifdef WINDOWS
#include <Windows.h>
#else
#include <pthread.h>
#endif

struct cr_mutex {
#ifdef WINDOWS
	LPCRITICAL_SECTION lock;
#else
	pthread_mutex_t lock; // = PTHREAD_MUTEX_INITIALIZER;
#endif
};


struct cr_mutex *mutex_create(void);

void mutex_destroy(struct cr_mutex *m);

void mutex_lock(struct cr_mutex *m);

void mutex_release(struct cr_mutex *m);
