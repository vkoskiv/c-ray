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

// TODO: How do I want to deal with system headers? I'd rather not #include string.h here.
// string.h
extern void *memcpy (void *__restrict __dest, const void *__restrict __src,
		     size_t __n) __THROW __nonnull ((1, 2));
// stdlib.h
extern void *malloc (size_t __size) __THROW __attribute_malloc__
     __attribute_alloc_size__ ((1)) __wur;
extern void *realloc (void *__ptr, size_t __size)
     __THROW __attribute_warn_unused_result__ __attribute_alloc_size__ ((2));
extern void free (void *__ptr) __THROW;

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

// --- decl v_mutex (Mutexes, pthreads & win32)
#if defined(WINDOWS)
	#include <Windows.h>
#else
	#include <pthread.h>
#endif

// FIXME: The struct is exposed here because thread.c guts access lock when passing to pthread_cond_wait.
struct v_mutex {
#if defined(WINDOWS)
	LPCRITICAL_SECTION lock;
#else
	pthread_mutex_t lock;
#endif
};

struct v_mutex *v_mutex_create(void);
void v_mutex_destroy(struct v_mutex *);
void v_mutex_lock(struct v_mutex *);
void v_mutex_release(struct v_mutex *);

#ifdef __cplusplus
}
#endif

#ifdef V_IMPLEMENTATION
#undef V_IMPLEMENTATION

// Implementation

#ifdef __cplusplus
extern "C" {
#endif

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
// --- impl v_sync (Sync primitives (mutex, rwlock))(c-ray)

// --- impl v_mutex (Mutexes, pthreads & win32)
struct v_mutex *v_mutex_create(void) {
	struct v_mutex *m = calloc(1, sizeof(*m));
#if defined(WINDOWS)
	InitializeCriticalSection(&m->lock);
#else
	m->lock = (pthread_mutex_t)PTHREAD_MUTEX_INITIALIZER;
#endif
	return m;
}

void v_mutex_destroy(struct v_mutex *m) {
	if (!m)
		return;
#if defined(WINDOWS)
	DeleteCriticalSection(m->lock);
#else
	pthread_mutex_destroy(&m->lock);
#endif
	free(m);
}

void v_mutex_lock(struct v_mutex *m) {
#if defined(WINDOWS)
	EnterCriticalSection(&m->lock);
#else
	pthread_mutex_lock(&m->lock);
#endif
}

void v_mutex_release(struct v_mutex *m) {
#if defined(WINDOWS)
	LeaveCriticalSection(&m->lock);
#else
	pthread_mutex_unlock(&m->lock);
#endif
}
// --- impl v_thread (Thread )
// --- impl job_queue

#ifdef __cplusplus
}
#endif

#endif // V_IMPLEMENTATION

#endif // _V_H_INCLUDED
