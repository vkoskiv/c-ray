//
//  thread.c
//  c-ray
//
//  Created by Valtteri on 29.3.2020.
//  Copyright © 2020-2024 Valtteri Koskivuori. All rights reserved.
//

#include <stdbool.h>
#include <stdint.h>

#include "thread.h"
#include "../logging.h"

/*
	cond stuff is based on:
	https://nachtimwald.com/2019/04/05/cross-platform-thread-wrapper/
*/

// Multiplatform thread stub
#ifdef WINDOWS
DWORD WINAPI thread_stub(LPVOID arg) {
#else
void *thread_stub(void *arg) {
#endif
	struct cr_thread copy = *(struct cr_thread *)arg;
	free(arg);
	return copy.thread_fn(copy.user_data);
}

void thread_wait(struct cr_thread *t) {
#ifdef WINDOWS
	WaitForSingleObjectEx(t->thread_handle, INFINITE, FALSE);
	CloseHandle(t->thread_handle);
#else
	if (pthread_join(t->thread_id, NULL)) {
		logr(warning, "Thread frozen! (This shouldn't happen.)");
	}
#endif
}

// NOTE: To be able to pass in a stack-local cr_thread struct, we do a temporary
// allocation here to extend the lifetime.

int thread_create_detach(struct cr_thread *t) {
#ifdef CR_SIMULATE_NOTHREADS
	return -1;
#endif
	if (!t) return -1;
	struct cr_thread *temp = calloc(1, sizeof(*temp));
	*temp = *t;
#ifdef WINDOWS
	t->thread_handle = CreateThread(NULL, 0, thread_stub, temp, 0, &t->thread_id);
	CloseHandle(t->thread_handle);
	return 0;
#else
	int rc = pthread_create(&t->thread_id, NULL, thread_stub, temp);
	if (rc)
		return rc;
	pthread_detach(t->thread_id);
	return 0;
#endif
}

int thread_start(struct cr_thread *t) {
#ifdef CR_SIMULATE_NOTHREADS
	return -1;
#endif
	if (!t) return -1;
	struct cr_thread *temp = calloc(1, sizeof(*temp));
	*temp = *t;
#ifdef WINDOWS
	t->thread_handle = CreateThread(NULL, 0, thread_stub, temp, 0, &t->thread_id);
	if (t->thread_handle == NULL) return -1;
	return 0;
#else
	pthread_attr_t attribs;
	pthread_attr_init(&attribs);
	pthread_attr_setdetachstate(&attribs, PTHREAD_CREATE_JOINABLE);
	int rc = pthread_create(&t->thread_id, &attribs, thread_stub, temp);
	pthread_attr_destroy(&attribs);
	return rc;
#endif
}

int thread_cond_init(struct cr_cond *cond) {
	if (!cond) return -1;
#ifdef WINDOWS
	InitializeConditionVariable(&cond->cond);
#else
	pthread_cond_init(&cond->cond, NULL);
#endif
	return 0;
}

int thread_cond_destroy(struct cr_cond *cond) {
	if (!cond) return -1;
#ifndef WINDOWS
	pthread_cond_destroy(&cond->cond);
#endif
	return 0;
}

int thread_cond_wait(struct cr_cond *cond, struct cr_mutex *mutex) {
	if (!cond || !mutex) return -1;
#ifdef WINDOWS
	return thread_cond_timed_wait(cond, mutex, NULL);
#else
	return pthread_cond_wait(&cond->cond, &mutex->lock);
#endif
}

#ifdef WINDOWS
static DWORD timespec_to_ms(const struct timespec *absolute_time) {
	if (!absolute_time) return INFINITE;
	DWORD t = ((absolute_time->tv_sec - time(NULL)) * 1000) + (absolute_time->tv_nsec / 1000000);
	return t < 0 ? 1 : t;
}
#endif

void ms_to_timespec(struct timespec *ts, unsigned int ms) {
	if (!ts) return;
	ts->tv_sec = (ms / 1000) + time(NULL);
	ts->tv_nsec = (ms % 1000) * 1000000;
}

int thread_cond_timed_wait(struct cr_cond *cond, struct cr_mutex *mutex, const struct timespec *absolute_time) {
	if (!cond || !mutex) return -1;
#ifdef WINDOWS
	if (!SleepConditionVariableCS(&cond->cond, &mutex->lock, timespec_to_ms(absolute_time)))
		return -1;
#else
	return pthread_cond_timedwait(&cond->cond, &mutex->lock, absolute_time);
#endif
	return 0;
}

int thread_cond_signal(struct cr_cond *cond) {
	if (!cond) return -1;
#ifdef WINDOWS
	WakeConditionVariable(&cond->cond);
	return 0;
#else
	return pthread_cond_signal(&cond->cond);
#endif
}

int thread_cond_broadcast(struct cr_cond *cond) {
	if (!cond) return -1;
#ifdef WINDOWS
	WakeAllConditionVariable(&cond->cond);
	return 0;
#else
	return pthread_cond_broadcast(&cond->cond);
#endif
}

#ifdef WINDOWS
/*
	Windows will presumably still suffer from false sharing, but
	their API is what it is :/
*/
struct cr_rwlock {
	SRWLOCK lock;
	bool exclusive;
};
#endif

struct cr_rwlock *thread_rwlock_init(void) {
#ifdef WINDOWS
	struct cr_rwlock *lock = malloc(sizeof(*lock));
	InitializeSRWLock(&lock->lock);
	lock->exclusive = false;
	return lock;
#else
	pthread_rwlock_t *lock = malloc(sizeof(*lock));
	int ret = pthread_rwlock_init(lock, NULL);
	if (ret) {
		free(lock);
		return NULL;
	}
	return (struct cr_rwlock *)lock;
#endif
}

int thread_rwlock_destroy(struct cr_rwlock *lock) {
	if (!lock)
		return -1;
#ifdef WINDOWS
	free(lock);
	return 0;
#else
	pthread_rwlock_destroy((pthread_rwlock_t *)lock);
	free(lock);
	return 0;
#endif
}

int thread_rwlock_rdlock(struct cr_rwlock *lock) {
	if (!lock)
		return -1;
#ifdef WINDOWS
	AcquireSRWLockShared(&lock->lock);
	return 0;
#else
	return pthread_rwlock_rdlock((pthread_rwlock_t *)lock);
#endif
}

int thread_rwlock_wrlock(struct cr_rwlock *lock) {
	if (!lock)
		return -1;
#ifdef WINDOWS
	AcquireSRWLockExclusive(&lock->lock);
	lock->exclusive = true;
	return 0;
#else
	return pthread_rwlock_wrlock((pthread_rwlock_t *)lock);
#endif
}

int thread_rwlock_unlock(struct cr_rwlock *lock) {
	if (!lock)
		return -1;
#ifdef WINDOWS
	if (lock->exclusive) {
		lock->exclusive = false;
		ReleaseSRWLockExclusive(&lock->lock);
	} else {
		ReleaseSRWLockShared(&lock->lock);
	}
	return 0;
#else
	return pthread_rwlock_unlock((pthread_rwlock_t *)lock);
#endif
}

