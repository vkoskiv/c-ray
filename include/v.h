/*
 * v.h - My collection of useful bits for writing C programs.
 *
 * You may use this, but I can't guarantee you'll find it useful.
 *
 * Remember to #define V_IMPLEMENTATION before #including v.h in
 * a single .c/.cpp file of your choosing.
 *
 * Copyright (c) 2025, Valtteri Koskivuori
 * SPDX-License-Identifier: MIT
 */

#ifndef _V_H_INCLUDED
#define _V_H_INCLUDED 1

#include <stddef.h>
#include <stdint.h>

// --- decl common

#ifdef __GNUC__
	#define V_UNUSED __attribute__((unused))
#else
	#define V_UNUSED
#endif

// Header

#ifdef __cplusplus
extern "C" {
#endif

// --- decl v_sys (System capabilities)

int v_sys_get_cores(void);

// --- decl v_mod (Runtime module loading)
typedef void * v_mod_t;
v_mod_t v_mod_load(const char *filename);
void *v_mod_sym(v_mod_t handle, const char *name);
char *v_mod_error(void);
int v_mod_close(v_mod_t handle);

// --- decl v_ilist (Intrusive doubly-linked list)
#define v_ilist_get(ptr, type, field) ((type *)((void *)(ptr) - offsetof(type, field)))

struct v_ilist {
	struct v_ilist *prev;
	struct v_ilist *next;
};
typedef struct v_ilist v_ilist;

void v_ilist_init(v_ilist *root);
void v_ilist_prepend(v_ilist *next, v_ilist *node);
void v_ilist_append(v_ilist *prev, v_ilist *node);
void v_ilist_foreach(v_ilist *head, int (*cb)(v_ilist *elem, void *ctx), void *ctx);
void v_ilist_remove(v_ilist *node);

// --- decl v_timer (Simple timers)

#ifdef WINDOWS
	typedef struct timeval {
		long tv_sec;
		long tv_usec;
	} TIMEVAL, *PTIMEVAL, *LPTIMEVAL;
#else
	#include <sys/time.h> // FIXME: Check
#endif

typedef struct timeval v_timer;

void v_timer_start(v_timer *t);
long v_timer_get_ms(v_timer t);
long v_timer_get_us(v_timer t);
void v_timer_sleep_ms(int ms);

// --- decl v_arr (Dynamic arrays)

#ifndef V_ARR_START_SIZE
#define V_ARR_START_SIZE 16
#endif

static inline size_t grow_x_1_5(size_t capacity, size_t elem_size) {
	if (capacity == 0)
		return V_ARR_START_SIZE;
	if (capacity > SIZE_MAX / (size_t)(1.5 * elem_size))
		return 0;
	return capacity + (capacity / 2);
}

static inline size_t grow_x_2(size_t capacity, size_t elem_size) {
	if (capacity == 0)
		return V_ARR_START_SIZE;
	if (capacity > SIZE_MAX / (2 * elem_size))
		return 0;
	return capacity * 2;
}

#include <string.h>
#include <stdlib.h>

#define v_arr_def(T) \
	struct T##_arr { \
		T *items; \
		size_t count; \
		size_t capacity; \
		size_t (*grow_fn)(size_t capacity, size_t item_size); \
		void   (*elem_free)(T *item); \
	}; \
	static inline size_t V_UNUSED T##_arr_add(struct T##_arr *a, const T value) { \
		if (a->count >= a->capacity) { \
			size_t new_capacity = a->grow_fn ? a->grow_fn(a->capacity, sizeof(*a->items)) : grow_x_2(a->capacity, sizeof(*a->items)); \
			a->items = realloc(a->items, sizeof(*a->items) * new_capacity); \
			a->capacity = new_capacity; \
		} \
		a->items[a->count] = value; \
		return a->count++; \
	} \
	static inline size_t V_UNUSED T##_arr_add_n(struct T##_arr *a, const T *values, size_t n) { \
		if ((a->count + n) >= a->capacity) { \
			size_t new_capacity = a->capacity + (a->capacity - a->count) + n; \
			a->items = realloc(a->items, sizeof(*a->items) * new_capacity); \
			a->capacity = new_capacity; \
		} \
		memcpy(a->items + a->count, values, n * sizeof(*a->items)); \
		return a->count += n; \
	} \
	static inline void V_UNUSED T##_arr_trim(struct T##_arr *a) { \
		if (!a || a->count >= a->capacity) return; \
		T *new = malloc(a->count * sizeof(*a)); \
		memcpy(new, a->items, a->count * sizeof(*new)); \
		free(a->items); \
		a->items = new; \
		a->capacity = a->count; \
	} \
	static inline struct T##_arr V_UNUSED T##_arr_copy(const struct T##_arr a) { \
		if (!a.items) return (struct T##_arr){ 0 }; \
		struct T##_arr c = a; \
		c.items = malloc(a.count * sizeof(*a.items)); \
		memcpy(c.items, a.items, a.count * sizeof(*a.items)); \
		return c; \
	} \
	static inline void V_UNUSED T##_arr_free(struct T##_arr *a) { \
		if (!a) return; \
		if (a->elem_free) { \
			for (size_t i = 0; i < a->count; ++i) \
				a->elem_free(&a->items[i]);\
		}\
		if (a->items) free(a->items); \
		a->items = NULL; \
		a->capacity = 0; \
		a->count = 0; \
	} \
	static inline void V_UNUSED T##_arr_join(struct T##_arr *a, struct T##_arr *b) { \
		if (!a || !b) return; \
		for (size_t i = 0; i < b->count; ++i) \
			T##_arr_add(a, b->items[i]); \
		T##_arr_free(b); \
	}

// --- decl v_mem (Arena allocator) (TODO)

// --- decl v_sync (Sync primitives (mutex, rwlock, condition variables), pthreads & win32)
#if defined(WINDOWS)
	#include <Windows.h>
	#include <time.h>
#else
	#include <pthread.h>
#endif

struct v_mutex;
typedef struct v_mutex v_mutex;
v_mutex *v_mutex_create(void);
void v_mutex_destroy(v_mutex *);
void v_mutex_lock(v_mutex *);
void v_mutex_release(v_mutex *);

struct v_cond;
typedef struct v_cond v_cond;
v_cond *v_cond_create(void);
void v_cond_destroy(v_cond *);
int v_cond_wait(v_cond *, v_mutex *);
int v_cond_timedwait(v_cond *, v_mutex *, const struct timespec *);
int v_cond_signal(v_cond *);    // Wake one thread waiting on v_cond
int v_cond_broadcast(v_cond *); // Wake all threads waiting on v_cond

struct v_rwlock;
typedef struct v_rwlock v_rwlock;
v_rwlock *v_rwlock_create(void);
void v_rwlock_destroy(v_rwlock *);
int v_rwlock_read_lock(v_rwlock *);
int v_rwlock_write_lock(v_rwlock *);
int v_rwlock_unlock(v_rwlock *);

// --- decl v_thread (Threading abstraction, pthreads & win32)
#if defined(WINDOWS)
	#include <stdbool.h>
	#include <Windows.h>
#else
	#include <pthread.h>
#endif

// TODO: Maybe wrap the platform-specific data in an opaque struct?
struct v_thread {
#if defined(WINDOWS)
	HANDLE thread_handle;
	DWORD thread_id;
#else
	pthread_t thread_id;
#endif
	void *ctx;                  // Thread context, this gets passed to thread_fn
	void *(*thread_fn)(void *); // The function to run in this thread
};
typedef struct v_thread v_thread;

enum v_thread_type {
	v_thread_type_joinable = 0,
	v_thread_type_detached,
};

int v_thread_start(v_thread *, enum v_thread_type);
void v_thread_exit(void *);
void *v_thread_wait(v_thread *);


#ifdef __cplusplus
}
#endif

#ifdef V_IMPLEMENTATION
#undef V_IMPLEMENTATION

// Implementation

#ifdef __cplusplus
extern "C" {
#endif

// --- impl v_sys (System capabilities)

#ifdef __APPLE__
typedef unsigned int u_int;
typedef unsigned char u_char;
typedef unsigned short u_short;
#include <sys/param.h>
#include <sys/sysctl.h>
#elif _WIN32
#include <windows.h>
#elif __linux__ || __COSMOPOLITAN__
#include <unistd.h>
#endif

int v_sys_get_cores(void) {
#if defined(__APPLE__)
	int nm[2];
	size_t len = 4;
	uint32_t count;

	nm[0] = CTL_HW; nm[1] = HW_AVAILCPU;
	sysctl(nm, 2, &count, &len, NULL, 0);

	if (count < 1) {
		nm[1] = HW_NCPU;
		sysctl(nm, 2, &count, &len, NULL, 0);
		if (count < 1) {
			count = 1;
		}
	}
	return (int)count;
#elif defined(_WIN32)
	SYSTEM_INFO sysInfo;
	GetSystemInfo(&sysInfo);
	return sysInfo.dwNumberOfProcessors;
#elif defined(__linux__) || defined(__COSMOPOLITAN__)
	return (int)sysconf(_SC_NPROCESSORS_ONLN);
#else
#warning "Unknown platform, v_sys_get_cores() will return 1. Let vkoskiv know about this, please."
	return 1;
#endif
}

// --- impl v_mod (Runtime library loading) (c-ray) ---
#if defined(WINDOWS)
	#include <windows.h>
	#include <libloaderapi.h>
#elif defined(__COSMOPOLITAN__)
	#define _COSMO_SOURCE
	#include "libc/dlopen/dlfcn.h"
#else
	#include <dlfcn.h>
#endif

v_mod_t v_mod_load(const char *filename) {
#if defined(WINDOWS)
	return (void *)LoadLibraryA(filename);
#elif defined(__COSMOPOLITAN__)
	return cosmo_dlopen(filename, RTLD_LAZY);
#else
	return dlopen(filename, RTLD_LAZY);
#endif
}

void *v_mod_sym(v_mod_t handle, const char *name) {
#if defined(WINDOWS)
	return (void *)GetProcAddress((HMODULE)handle, name);
#elif defined(__COSMOPOLITAN__)
	return cosmo_dlsym(handle, name);
#else
	return dlsym(handle, name);
#endif
}

char *v_mod_error(void) {
#if defined(WINDOWS)
	return NULL;
#elif defined(__COSMOPOLITAN__)
	return cosmo_dlerror();
#else
	return dlerror();
#endif
}

int v_mod_close(v_mod_t handle) {
#if defined(WINDOWS)
	return (int)FreeLibrary((HMODULE)handle);
#elif defined(__COSMOPOLITAN__)
	return cosmo_dlclose(handle);
#else
	return dlclose(handle);
#endif
}

// --- impl v_ilist (Intrusive doubly-linked list)

void v_ilist_init(v_ilist *root) {
	root->prev = root;
	root->next = root;
}

#define _v_ilist_LINK \
	prev->next = node; \
	node->prev = prev; \
	next->prev = node; \
	node->next = next;

void v_ilist_prepend(v_ilist *next, v_ilist *node) {
	v_ilist *prev = next->prev;
	_v_ilist_LINK
}

void v_ilist_append(v_ilist *prev, v_ilist *node) {
	v_ilist *next = prev->next;
	_v_ilist_LINK
}

void v_ilist_foreach(v_ilist *head, int (*cb)(v_ilist *elem, void *ctx), void *ctx) {
	v_ilist *n = head;
	while (n != head) {
		cb(n, ctx);
		n = n->next;
	}
}

void v_ilist_remove(v_ilist *node) {
	v_ilist *prev = node->prev;
	v_ilist *next = node->next;
	prev->next = next;
	next->prev = prev;
}

// --- impl v_timer (Simple timers)

#ifdef WINDOWS
static int gettimeofday(struct timeval * tp, struct timezone * tzp) {
	// Note: some broken versions only have 8 trailing zero's, the correct epoch has 9 trailing zero's
	// This magic number is the number of 100 nanosecond intervals since January 1, 1601 (UTC)
	// until 00:00:00 January 1, 1970
	static const uint64_t EPOCH = ((uint64_t) 116444736000000000ULL);
	
	SYSTEMTIME  system_time;
	FILETIME    file_time;
	uint64_t    time;
	
	GetSystemTime( &system_time );
	SystemTimeToFileTime( &system_time, &file_time );
	time =  ((uint64_t)file_time.dwLowDateTime )      ;
	time += ((uint64_t)file_time.dwHighDateTime) << 32;
	
	tp->tv_sec  = (long) ((time - EPOCH) / 10000000L);
	tp->tv_usec = (long) (system_time.wMilliseconds * 1000);
	return 0;
}
#endif

void v_timer_start(v_timer *t) {
	// FIXME: Re-entrant?
	gettimeofday(t, NULL);
}

long v_timer_get_ms(v_timer t) {
	v_timer now;
	gettimeofday(&now, NULL);
	return 1000 * (now.tv_sec - t.tv_sec) + ((now.tv_usec - t.tv_usec) / 1000);
}

long v_timer_get_us(v_timer t) {
	v_timer now;
	gettimeofday(&now, NULL);
	return ((now.tv_sec - t.tv_sec) * 1000000) + (now.tv_usec - t.tv_usec);
}

#ifdef __linux__
#define _BSD_SOURCE
#include <unistd.h>
#endif

void v_timer_sleep_ms(int ms) {
#ifdef WINDOWS
	Sleep(ms);
#elif __APPLE__
	struct timespec ts;
	ts.tv_sec = ms / 1000;
	ts.tv_nsec = (ms % 1000) * 1000000;
	nanosleep(&ts, NULL);
#else
	struct timeval tv = { 0 };
	tv.tv_sec = ms / 1000;
	tv.tv_usec = ms % 1000 * 1000;
	select(0, NULL, NULL, NULL, &tv);
#endif
}

// --- impl v_ht (Hash table w/ FNV)(c-ray)
// --- impl v_cbuf (Circular buffers for running averages) (refmon)
// --- impl v_arr (Dynamic arrays) (c-ray?)

#include <stdlib.h>
#include <string.h>

v_arr_def(int)
v_arr_def(float)
v_arr_def(size_t)


// --- impl v_mem (Arena allocator) (TODO)

// --- impl v_sync (Sync primitives (mutex, rwlock, condition variables), pthreads & win32)

struct v_mutex {
#if defined(WINDOWS)
	LPCRITICAL_SECTION lock;
#else
	pthread_mutex_t lock;
#endif
};

v_mutex *v_mutex_create(void) {
	v_mutex *m = calloc(1, sizeof(*m));
	if (!m)
		return NULL;
#if defined(WINDOWS)
	InitializeCriticalSection(&m->lock);
#else
	m->lock = (pthread_mutex_t)PTHREAD_MUTEX_INITIALIZER;
#endif
	return m;
}

void v_mutex_destroy(v_mutex *m) {
	if (!m)
		return;
#if defined(WINDOWS)
	DeleteCriticalSection(m->lock);
#else
	pthread_mutex_destroy(&m->lock);
#endif
	free(m);
}

void v_mutex_lock(v_mutex *m) {
#if defined(WINDOWS)
	EnterCriticalSection(&m->lock);
#else
	pthread_mutex_lock(&m->lock);
#endif
}

void v_mutex_release(v_mutex *m) {
#if defined(WINDOWS)
	LeaveCriticalSection(&m->lock);
#else
	pthread_mutex_unlock(&m->lock);
#endif
}

struct v_cond {
#if defined(WINDOWS)
	PCONDITION_VARIABLE cond;
#else
	pthread_cond_t cond;
#endif
};

v_cond *v_cond_create(void) {
	struct v_cond *c = calloc(1, sizeof(*c));
	if (!c)
		return NULL;
#if defined(WINDOWS)
	InitializeConditionVariable(&c->cond);
#else
	// TODO: What's that second param?
	pthread_cond_init(&c->cond, NULL);
#endif
	return c;
}

void v_cond_destroy(v_cond *c) {
	if (!c)
		return;
#if defined(WINDOWS)
	// TODO: Check if we need to do anything
#else
	pthread_cond_destroy(&c->cond);
	free(c);
#endif
}

int v_cond_wait(v_cond *c, v_mutex *m) {
	if (!c || !m)
		return -1;
#if defined(WINDOWS)
	return v_cond_timedwait(c, m, NULL);
#else
	return pthread_cond_wait(&c->cond, &m->lock);
#endif
}

#if defined(WINDOWS)
static DWORD timespec_to_ms(const struct timespec *absolute_time) {
	if (!absolute_time)
		return INFINITE;
	DWORD t = ((absolute_time->tv_sec - time(NULL)) * 1000) + (absolute_time->tv_nsec / 1000000);
	return t < 0 ? 1 : t;
}
#endif

int v_cond_timedwait(v_cond *c, v_mutex *m, const struct timespec *ts) {
	if (!c || !m)
		return -1;
#if defined(WINDOWS)
	if (!SleepConditionVariableCS(&c->cond, &m->lock, timespec_to_ms(ts)))
		return -1;
#else
	return pthread_cond_timedwait(&c->cond, &m->lock, ts);
#endif
}

int v_cond_signal(v_cond *c) {
	if (!c)
		return -1;
#if defined(WINDOWS)
	// TODO: Check return val?
	WakeConditionVariable(&c->cond);
	return 0;
#else
	return pthread_cond_signal(&c->cond);
#endif
}

int v_cond_broadcast(v_cond *c) {
	if (!c)
		return -1;
#if defined(WINDOWS)
	WakeAllConditionVariable(&c->cond);
	return 0;
#else
	return pthread_cond_broadcast(&c->cond);
#endif
}

#if defined(WINDOWS)
struct v_rwlock {
	SRWLOCK lock;
	bool exclusive;
};
#endif

v_rwlock *v_rwlock_create(void) {
#if defined(WINDOWS)
	v_rwlock *l = malloc(sizeof(*l));
	InitializeSRWLock(&l->lock);
	l->exclusive = false;
	return l;
#else
	pthread_rwlock_t *l = malloc(sizeof(*l));
	int ret = pthread_rwlock_init(l, NULL);
	if (ret) {
		free(l);
		return NULL;
	}
	return (v_rwlock *)l;
#endif
}

void v_rwlock_destroy(v_rwlock *l) {
	if (!l)
		return;
#if defined(WINDOWS)
	free(l);
#else
	pthread_rwlock_destroy((pthread_rwlock_t *)l);
	free(l);
#endif
}

int v_rwlock_read_lock(v_rwlock *l) {
	if (!l)
		return -1;
#if defined(WINDOWS)
	AcquireSRWLockShared(&l->lock);
	return 0;
#else
	return pthread_rwlock_rdlock((pthread_rwlock_t *)l);
#endif
}

int v_rwlock_write_lock(v_rwlock *l) {
	if (!l)
		return -1;
#if defined(WINDOWS)
	AcquireSRWLockExclusive(&l->lock);
	l->exclusive = true;
	return 0;
#else
	return pthread_rwlock_wrlock((pthread_rwlock_t *)l);
#endif
}

int v_rwlock_unlock(v_rwlock *l) {
	if (!l)
		return -1;
#if defined(WINDOWS)
	if (l->exclusive) {
		l->exclusive = false;
		ReleaseSRWLockExclusive(&l->lock);
	} else {
		ReleaseSRWLockShared(&l->lock);
	}
	return 0;
#else
	return pthread_rwlock_unlock((pthread_rwlock_t *)l);
#endif
}

// --- impl v_thread (Threading abstraction, pthreads & win32)

#if defined(WINDOWS)
static DWORD WINAPI thread_stub(LPVOID arg) {
#else
static void *thread_stub(void *arg) {
#endif
	v_thread copy = *((struct v_thread *)arg);
	free(arg);
	return copy.thread_fn(copy.ctx);
}

int v_thread_start(v_thread *t, enum v_thread_type type) {
	if (!t)
		return -1;
	// NOTE: We extend the lifetime with this allocation, so users can pass in
	// references to locals like a designated initializer like this:
	// int rc = v_thread_start(&(struct v_thread){ .thread_fn = foobar_fn, ctx = foobar_ctx });
	// The allocation is freed automatically by thread_stub.
	// TODO: Look into a solution that obviates a heap alloc
	v_thread *temp = calloc(1, sizeof(*temp));
	*temp = *t;
#if defined(WINDOWS)
	t->thread_handle = CreateThread(NULL, 0, thread_stub, temp, 0, &t->thread_id);
	if (t->thread_handle == NULL)
		return -1;
	if (type == v_thread_type_detached)
		CloseHandle(t->thread_handle);
	return 0;
#else
	pthread_attr_t attribs;
	pthread_attr_init(&attribs);
	switch (type) {
	case v_thread_type_joinable:
		pthread_attr_setdetachstate(&attribs, PTHREAD_CREATE_JOINABLE);
		break;
	case v_thread_type_detached:
		pthread_attr_setdetachstate(&attribs, PTHREAD_CREATE_DETACHED);
		break;
	}
	int rc = pthread_create(&t->thread_id, &attribs, thread_stub, temp);
	pthread_attr_destroy(&attribs);
	return rc;
#endif
}

void v_thread_exit(void *arg) {
#if defined(WINDOWS)
	return; // FIXME
#else
	pthread_exit(arg);
#endif
}

void *v_thread_wait(v_thread *t) {
#if defined(WINDOWS)
	WaitForSingleObjectEx(t->thread_handle, INFINITE, FALSE);
	CloseHandle(t->thread_handle);
	return NULL;
#else
	int ret = 0;
	void *thread_ret = NULL;
	ret = pthread_join(t->thread_id, &thread_ret);
	if (!ret && thread_ret)
		return thread_ret;
	return NULL;
#endif
}

// --- impl job_queue

#ifdef __cplusplus
}
#endif

#endif // V_IMPLEMENTATION

#endif // _V_H_INCLUDED
