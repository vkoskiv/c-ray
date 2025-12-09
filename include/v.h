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
 // TODO: Prettify comments (e.g. cosmopolitan. there was a tool for this, I forget the name!)

 /*
	Feature list, search for:
	- '--- decl <name>' to get to the API
	- '--- impl <name>' to get to the implementation

	- v_sys: Query system information       (*nix, win32)
	- v_mod: Dynamic module support         (*nix, win32)
	- v_ilist: Intrusive doubly-linked list
	- v_timer: Simple timer gadget
	- v_dir: TODO: Directory utilities (port from refmon)
	- v_ht: TODO: Hash-table (port from c-ray)
	- v_cbuf: TODO: Circular buffer (port from refmon)
	- v_arr: Dynamic arrays
	- v_str: TODO UTF-8 String library
	- v_arena: Arena allocator
	- v_sync: Sync primitives (mutex, rwlock, condition variables), (pthreads, win32)
	- v_thread: Threading abstraction & thread pool (pthreads, win32)
	- v_threadpool: Thread pool + job queue
 */

#ifndef _V_H_INCLUDED
#define _V_H_INCLUDED 1

/* Platform-specific tweaks */
#if defined(__NetBSD__) && !defined(_NETBSD_SOURCE) /* stack_t */
	#define _NETBSD_SOURCE
#endif

/* v.h depends on C99 */
#if !defined(__STDC_VERSION__) || __STDC_VERSION__ < 199901L
	#error "The v.h library requires C99 or later."
#endif

/* v.h depends on pthread_rwlock_t and other POSIX goodies */
#ifndef _POSIX_C_SOURCE
	#define _POSIX_C_SOURCE 200112L
#endif

#include <stddef.h>
#include <stdint.h>

// --- decl common

#if defined(__GNUC__)
	#define V_UNUSED __attribute__((unused))
#else
	#define V_UNUSED
#endif

#define v_offsetof(T, m) ((size_t)((char *)&((T *)1)->m - (char *)1))
#define v_alignof(T) v_offsetof(struct { char _; T x; }, x)
#define v_container_of(ptr, type, member) \
	((type *)((char *)(ptr) - v_offsetof(type, member)))

union v_max_align_t {
	char *p;
	double d;
	long double ld;
	long int i;
};
typedef union v_max_align_t v_max_align_t;

// Header

#ifdef __cplusplus
extern "C" {
#endif

// --- decl v_sys (System capabilities)

int v_sys_get_cores(void);

// --- decl v_mod (Runtime module loading)
typedef void * v_mod;
v_mod v_mod_load(const char *filename);
void *v_mod_sym(v_mod handle, const char *name);
char *v_mod_error(void);
int v_mod_close(v_mod handle);

// --- decl v_ilist (Intrusive doubly-linked list)
#define v_ilist_get(ptr, type, field) ((type *)((void *)(ptr) - offsetof(type, field)))

struct v_ilist {
	struct v_ilist *prev;
	struct v_ilist *next;
};
typedef struct v_ilist v_ilist;

void v_ilist_init(v_ilist *root);
v_ilist *v_ilist_prepend(v_ilist *next, v_ilist *node);
v_ilist *v_ilist_append(v_ilist *prev, v_ilist *node);
void v_ilist_foreach(v_ilist *head, int (*cb)(v_ilist *elem, void *ctx), void *ctx);
void v_ilist_remove(v_ilist *node);
void v_ilist_destroy(v_ilist *head, void (*dtor)(void *elem));
size_t v_ilist_elems(v_ilist *head);

#define v_ilist_for_each(list_head, T, elem) \
	for (T *elem = (T *)list_head, *end = NULL;\
	elem != end;\
	end = (end ? end : elem), elem = (T *)((v_ilist *)elem)->next)

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

v_timer v_timer_start(void);
long v_timer_get_ms(v_timer t);
long v_timer_get_us(v_timer t);
/*  Note: On Linux, this sleep will resume clock_nanosleep() to finish
	up the sleep if we happened to get a signal, such as SIGINT during sleep. */
void v_timer_sleep_ms(int ms);

// --- decl v_dir (Directory iteration goodies) (refmon)
// --- decl v_ht (Hash table w/ FNV)(c-ray)
// --- decl v_cbuf (Circular buffers for running averages) (refmon)
// --- decl v_arr (Dynamic arrays)

struct v_arr {
	size_t n;
	size_t cap;
	size_t elem_size;
	size_t (*grow_fn)(size_t cap, size_t elem_size);
	// v_mem *allocator; <- Used if not NULL, and this type casts to e.g. v_arena, v_stdalloc, v_mempool, etc.
	void (*elem_free)(void *elem);
};

#ifndef V_ARR_START_SIZE
#define V_ARR_START_SIZE 16
#endif

// TODO: Maybe have these out of line in impl?
static inline size_t v_arr_grow_linear(size_t capacity, size_t elem_size) {
	if (capacity == 0)
		return V_ARR_START_SIZE;
	if (capacity > SIZE_MAX / (size_t)(1.5 * elem_size))
		return 0;
	return capacity + (capacity / 2);
}

static inline size_t v_arr_grow_exponential(size_t capacity, size_t elem_size) {
	if (capacity == 0)
		return V_ARR_START_SIZE;
	if (capacity > SIZE_MAX / (2 * elem_size))
		return 0;
	return capacity * 2;
}

void *v__arr_do_grow(void *a, size_t elem_size, size_t n);
void v__arr_add_n(void *a, void *elems, size_t elem_size, size_t n);
void *v__arr_trim(void *a);
void *v__arr_copy(const void *const a);
void v__arr_free(void *a);
#define v__arr_head(A) ((struct v_arr *)(A) - 1)
#define v__arr_grow(A, N) ((A) = v__arr_do_grow((A), sizeof(*A), (N)))
#define v__arr_ensure(A, N) \
	(((!A) || v__arr_head(A)->n + N > v__arr_head(A)->cap) ? (v__arr_grow(A, N), 0) : 0)


#define v_arr_cap(A) ((A) ? v__arr_head(A)->cap : 0)
#define v_arr_len(A) ((A) ? v__arr_head(A)->n : 0)
#define v_arr_add(A, ...) \
	(v__arr_ensure((A), 1), (A)[v__arr_head(A)->n] = (__VA_ARGS__), v__arr_head(A)->n++)
#define v_arr_add_n(A, elems, N) \
	(v__arr_ensure((A), N), v__arr_add_n((A), elems, sizeof(elems[0]), (N)))
// TODO: It's quite easy to forget & when using v_arr_trim().
// Use the v_arr_free() pattern and directly assign result of v__arr_trim() to A
#define v_arr_trim(A) \
	((A) ? ((A) = v__arr_trim((A)), 0) : 0)
#define v_arr_copy(A) \
	((A) ? v__arr_copy((A)) : NULL)
#define v_arr_free(A) \
	(((A) ? (v__arr_free((A)), 0) : 0), (A) = NULL)
#define v_arr_join(A, B) \
	((A) && (B) ? (v_arr_add_n((A), (B), v_arr_len((B))), v_arr_free((B)), (A)) : NULL)
#define v_arr_set_grow_fn(A, fn) \
	(v__arr_ensure((A), 0), v__arr_head(A)->grow_fn = fn)

// --- decl v_str (UTF-8 checked strings)

struct v_str {
	const char *s;
	size_t bytes;
};
typedef struct v_str v_str;

v_str v_s(const char *);

struct v_hstr {
	char *s;
	size_t bytes;
};
typedef struct v_hstr v_hstr;

v_hstr v_h(const char *);
v_hstr v_sfmt(const char *fmt, ...);
void v_str_free(v_hstr s);

// --- decl v_arena (Arena allocator)

/*
	Largely inspired by:
	https://nullprogram.com/blog/2023/09/27/

	Usage example:

		struct foo {
			int a;
			char b;
		};

		void complex_nested_operation(v_arena a) {
			// ...
			struct foo *too_big = v_new(&a, struct foo, 1000 * 1000); // -> longjmp back to do_things() OOM handler
		}

		int do_things(v_arena a) {
			// Allocate single object
			struct foo *foo = v_new(&a, struct foo);
			// Allocate 128 objects
			struct foo *foo_list = v_new(&a, struct foo, 128);
			// Request NULL on OOM for individual allocation, so we can check and
			// handle the case manually. This could also be set in v_arena.flags and
			// it'll apply to all subsequent allocations from that arena.
			struct foo *sensitive_foo = v_new(&a, struct foo, 1, V_ARENA_SOFTFAIL);
			if (!sensitive_foo) {
				// <release locks/other resources>
				return 1;
			}
			// Another alternative, set an OOM handler for easy error handling
			// from comples/deeply nested operations that may OOM:
			v_arena_on_oom(a) {
				// OOMs from downstream will jump back here
				// Since we're passing v_arena by value, these can be nested, too.
				fprintf(stderr, "do_things: OOM in complex_nexted_operation(), bailing.\n");
				return 1;
			}
			complex_nested_operation(a);
			return 0;
		}

		int main(void) {
			// Allocate 1MiB arena from heap
			v_arena arena = v_arena_from_heap(1 * V_MiB);
			do_things(arena);
			v_arena_destroy(&arena);
		}

	We could also use a stack buffer, and set SOFTFAIL OOM strategy, so v_new()/v_put() return NULL on OOM:
		uint8_t arr[4 * V_KiB];
		v_arena arena = v_arena_from_arr(arr);
		arena.flags |= V_ARENA_SOFTFAIL;
		struct foo *foo_list = v_new(&arena, struct foo, 100);
		if (!foo_list) {
			// Setting V_ARENA_SOFTFAIL means we have to check each allocation the old-fashioned way
			return 1;
		}
		// use foo_list
*/

struct v_arena {
	uint8_t *alloc;
	uint8_t *beg;
	uint8_t *end;
	void *jmp_buf[5]; // x86_64: rbp, rip, rsp, 0, 0
	int flags;
};
typedef struct v_arena v_arena;

// v!v!v!v INTERNAL GUNK v!v!v!v
void *_v_arena_alloc(v_arena *a, ptrdiff_t size, ptrdiff_t align, ptrdiff_t count, int flags, void *data);
#define _v_newx(a, b, c, d, e, ...) e
#define _v_new2(arena, type)               (type *)_v_arena_alloc(arena, sizeof(type), v_alignof(type),     1,     0, NULL)
#define _v_new3(arena, type, count)        (type *)_v_arena_alloc(arena, sizeof(type), v_alignof(type), count,     0, NULL)
#define _v_new4(arena, type, count, flags) (type *)_v_arena_alloc(arena, sizeof(type), v_alignof(type), count, flags, NULL)
// ^!^!^!^ INTERNAL GUNK ^!^!^!^

	#define V_ARENA_NOZERO (1 << 0)
	#define V_ARENA_SOFTFAIL (1 << 1)
	#define V_ARENA_DO_LONGJMP (1 << 2)

	#define V_KiB 1024ull
	#define V_MiB (V_KiB * V_KiB)
	#define V_GiB (V_KiB * V_MiB)
	#define V_TiB (V_KiB * V_GiB)

	v_arena v_arena_from_buf(uint8_t *buf, ptrdiff_t capacity);
	#define v_arena_from_arr(array) v_arena_from_buf(array, sizeof(array))
	v_arena v_arena_from_heap(ptrdiff_t capacity);
	void v_arena_destroy(v_arena *a);

	// Very useful, but do not use if you received a pointer to v_arena
	// e.g. v_arena_on_oom((*arena_arg)) { ... } is a *bad* idea, and *will* blow up your stack.
	#define v_arena_on_oom(arena) if (arena.flags |= V_ARENA_DO_LONGJMP, __builtin_setjmp(arena.jmp_buf))

	// TODO: Maybe have these act on a generic allocator interface that uses e.g. v_arena/etc behind the scenes?
	#define v_new(...) _v_newx(__VA_ARGS__, _v_new4, _v_new3, _v_new2)(__VA_ARGS__)
	#define v_put(arena, type, ...) _v_arena_alloc(arena, sizeof(type), v_alignof(type), 1, 0, &__VA_ARGS__)

// --- decl v_mem (Composable memory allocators)

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
	#include <Windows.h>
#else
	#include <pthread.h>
#endif

struct v_thread;
typedef struct v_thread v_thread;

struct v_thread_ctx {
	void *ctx;                  // Thread context, this gets passed to thread_fn
	void *(*thread_fn)(void *); // The function to run in this thread
};
typedef struct v_thread_ctx v_thread_ctx;

enum v_thread_type {
	v_thread_type_joinable = 0,
	v_thread_type_detached,
};

/*
	NOTE: Return value of v_thread_start() is freed automatically when
	type == v_thread_type_detached, and should only be used to check
	for errors (NULL), and not stored. Passing it to v_thread_wait() is
	considered undefined.
	In the case type == v_thread_type_joinable, the required call to
	v_thread_wait() calls free() on v_thread *.
*/
v_thread *v_thread_create(v_thread_ctx c, enum v_thread_type type);
void *v_thread_wait_and_destroy(v_thread *);

// --- decl v_threadpool (Thread pool + job queue)

struct v_threadpool;
typedef struct v_threadpool v_threadpool;

/* NOTE: n_threads == 0 will default to system ncpu + 1 */
v_threadpool *v_threadpool_create(size_t n_threads);
void v_threadpool_destroy(v_threadpool *);

/* NOTE: If p == NULL, enqueue will run fn synchronously. */
int v_threadpool_enqueue(v_threadpool *p, void (*fn)(void *arg), void *arg);

/* Block until all currently queued jobs are complete. Calls to _enqueue() will
   block until this wait is finished. */
void v_threadpool_wait(v_threadpool *);

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

#if defined(__APPLE__)
	typedef unsigned int u_int;
	typedef unsigned char u_char;
	typedef unsigned short u_short;
	#include <sys/param.h>
	#include <sys/sysctl.h>
#elif defined(_WIN32)
	#include <windows.h>
#elif defined(__linux__) || defined(__COSMOPOLITAN__) || defined(__NetBSD__)
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
#elif defined(__linux__) || defined(__COSMOPOLITAN__) || defined(__NetBSD__)
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

v_mod v_mod_load(const char *filename) {
#if defined(WINDOWS)
	return (void *)LoadLibraryA(filename);
#elif defined(__COSMOPOLITAN__)
	return cosmo_dlopen(filename, RTLD_LAZY);
#else
	return dlopen(filename, RTLD_LAZY);
#endif
}

void *v_mod_sym(v_mod handle, const char *name) {
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

int v_mod_close(v_mod handle) {
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

v_ilist *v_ilist_prepend(v_ilist *next, v_ilist *node) {
	v_ilist *prev = next->prev;
	_v_ilist_LINK
	return node;
}

v_ilist *v_ilist_append(v_ilist *prev, v_ilist *node) {
	v_ilist *next = prev->next;
	_v_ilist_LINK
	return node;
}

void v_ilist_foreach(v_ilist *head, int (*cb)(v_ilist *elem, void *ctx), void *ctx) {
	v_ilist *n = head;
	do {
		cb(n, ctx);
		n = n->next;
	} while (n != head);
}

void v_ilist_remove(v_ilist *node) {
	if (!node)
		return;
	v_ilist *prev = node->prev;
	v_ilist *next = node->next;
	prev->next = next;
	next->prev = prev;
}

void v_ilist_destroy(v_ilist *head, void (*dtor)(void *elem)) {
	if (!head)
		return;
	if (head->next == head) {
		if (dtor)
			dtor(head);
		return;
	}
	v_ilist *tmp = head->next;
	head->next = head->next->next;
	if (dtor)
		dtor(tmp);
	v_ilist_destroy(head, dtor);
}

size_t v_ilist_elems(v_ilist *head) {
	size_t elems = 0;
	v_ilist_for_each(head, v_ilist, l) {
		elems++;
	}
	return elems;
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

v_timer v_timer_start(void) {
	v_timer timer = { 0 };
	gettimeofday(&timer, NULL);
	return timer;
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

#if defined(__linux__)
	#define _BSD_SOURCE
	#include <unistd.h>
	#include <errno.h>
#endif

void v_timer_sleep_ms(int ms) {
#if defined(WINDOWS)
	Sleep(ms);
#elif defined(__APPLE__)
	struct timespec ts;
	ts.tv_sec = ms / 1000;
	ts.tv_nsec = (ms % 1000) * 1000000;
	nanosleep(&ts, NULL);
#elif defined (__linux__)
	struct timespec ts = { .tv_sec = ms / 1000, .tv_nsec = (ms % 1000) * 1000 * 1000 };
	struct timespec rem;
	while (clock_nanosleep(CLOCK_MONOTONIC, 0, &ts, &rem) == EINTR) {
		// We received SIGINT which interrupts this sleep. Continue sleeping.
		ts = rem;
		rem = (struct timespec){ 0 };
	}
#else
	struct timeval tv = { 0 };
	tv.tv_sec = ms / 1000;
	tv.tv_usec = ms % 1000 * 1000;
	select(0, NULL, NULL, NULL, &tv);
#endif
}

// --- impl v_ht (Hash table w/ FNV)(c-ray)
// --- impl v_cbuf (Circular buffers for running averages) (refmon)
// --- impl v_arr (Dynamic arrays)

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

void *v__arr_do_grow(void *a, size_t elem_size, size_t n) {
	if (!a) {
		n = n < V_ARR_START_SIZE ? V_ARR_START_SIZE : n;
		size_t initial = sizeof(struct v_arr) + n * elem_size;
		struct v_arr *arr = calloc(1, initial);
		arr->cap = n;
		arr->elem_size = elem_size;
		return &arr[1];
	}
	struct v_arr *arr = v__arr_head(a);
	if (arr->n + n > arr->cap) {
		size_t new_capacity = arr->grow_fn ? arr->grow_fn(arr->cap, arr->elem_size) : v_arr_grow_exponential(arr->cap, arr->elem_size);
		arr = realloc(arr, arr->elem_size * new_capacity + sizeof(*arr));
		arr->cap = new_capacity;
	}
	return &arr[1];
}

#define v_arr_add_n___(A, items, N) \
	(v__arr_ensure((A), N), memcpy(A + v__arr_head(A)->n, items, N * sizeof(items[0])), v__arr_head(A)->n += N)

void v__arr_add_n(void *a, void *elems, size_t elem_size, size_t n) {
	v__arr_ensure(a, n);
	memcpy(a + v__arr_head(a)->n * elem_size, elems, n * elem_size);
	v__arr_head(a)->n += n;
}

void *v__arr_trim(void *a) {
	struct v_arr *arr = v__arr_head(a);
	if (arr->n == arr->cap)
		return a;
	arr->cap = arr->n;
	arr = realloc(arr, arr->elem_size * arr->cap + sizeof(*arr));
	return &arr[1];
}

void *v__arr_copy(const void *const a) {
	void *copy = v__arr_do_grow(NULL, v__arr_head(a)->elem_size, v_arr_cap(a));
	v__arr_head(copy)->n = v__arr_head(a)->n;
	memcpy(copy, a, v_arr_len(a) * v__arr_head(a)->elem_size);
	return copy;
}

void v__arr_free(void *a) {
	struct v_arr *arr = v__arr_head(a);
	// TODO: if (arr->allocator) ...
	free(arr);
}

// --- impl v_str (UTF-8 checked strings)

// static int s_is_utf8_cont(char c) {
// 	return (c & 0xC0) == 0x80;
// }

static size_t s_utf8_bytes(const char *s) {
	(void)s;
	return 0;
}

v_str v_s(const char *s) {
	return (v_str){ .s = s, .bytes = s_utf8_bytes(s) };
}

// --- impl v_arena (Arena allocator)

void *_v_arena_alloc(v_arena *a, ptrdiff_t size, ptrdiff_t align, ptrdiff_t count, int flags, void *data) {
	ptrdiff_t padding = -(uintptr_t)a->beg & (align - 1);
	ptrdiff_t available = a->end - a->beg - padding;
	// TODO: Original nullprogram impl had this first check
	// as available < 0, which seems wrong. Maybe look into that?
	if (available <= 0 || count > available / size) {
		if (a->flags & V_ARENA_DO_LONGJMP)
			__builtin_longjmp(a->jmp_buf, 1);
		else if ((flags & V_ARENA_SOFTFAIL) || (a->flags & V_ARENA_SOFTFAIL))
			return NULL;
		else
			abort();
	}
	void *p = a->beg + padding;
	ptrdiff_t bytes = count * size;
	a->beg += bytes + padding;
	if (data)
		return memcpy(p, data, size);
	return ((flags & V_ARENA_NOZERO) || (a->flags & V_ARENA_NOZERO)) ? p : memset(p, 0, bytes);
}

v_arena v_arena_from_buf(uint8_t *buf, ptrdiff_t capacity) {
	v_arena a = { 0 };
	a.beg = buf;
	a.end = a.beg ? a.beg + capacity : 0;
	return a;
}

// TODO: have internal logic to e.g. use mmap()/VirtualAlloc()
v_arena v_arena_from_heap(ptrdiff_t capacity) {
	uint8_t *buf = malloc(capacity);
	v_arena mem = v_arena_from_buf(buf, capacity);
	mem.alloc = buf;
	return mem;
}

void v_arena_destroy(v_arena *a) {
	if (!a)
		return;
	if (a->alloc) {
		free(a->alloc);
		a->alloc = NULL;
	}
	a->beg = a->end = 0;
}

// --- impl v_sync (Sync primitives (mutex, rwlock, condition variables), pthreads & win32)

// TODO: IDEA, simple allocator API like
struct v_alloc {
	void *(*malloc)(size_t);
	void *(*zalloc)(size_t, size_t);
	void (*free)(void *);
	// etc
};
// static struct v_alloc v_alloc_stdlib = {
// 	.malloc = malloc,
// 	.zalloc = calloc,
// 	.free = free,
// };
// And we could provide custom allocators:
// static v_alloc v_alloc_arena = {
// 	.ctx = v_arena,
// 	.alloc = v_arena_alloc,
// 	.cfree = NULL,
// };
// And then that could get passed into things, and defaults to NULL if no allocator
// struct v_mutex *v_mutex_create(struct v_alloc *a) {
// 	struct v_mutex *m = a ? a->zalloc(1, sizeof(*m)) : v_alloc_stdlib.zalloc(1, sizeof(*m));
// 	// etc, etc
// }

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
	char exclusive;
};
#endif

v_rwlock *v_rwlock_create(void) {
#if defined(WINDOWS)
	v_rwlock *l = malloc(sizeof(*l));
	InitializeSRWLock(&l->lock);
	l->exclusive = 0;
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
	l->exclusive = 1;
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
		l->exclusive = 0;
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

struct v_thread {
#if defined(WINDOWS)
	HANDLE thread_handle;
	DWORD thread_id;
#else
	pthread_t thread_id;
#endif
	enum v_thread_type type;
	struct v_thread_ctx thread_data;
	void *ret;
};

#if defined(WINDOWS)
static DWORD WINAPI thread_stub(LPVOID arg) {
#else
static void *thread_stub(void *arg) {
#endif
	v_thread *t = (struct v_thread *)arg;
	t->ret = t->thread_data.thread_fn(t->thread_data.ctx);
	if (t->type == v_thread_type_detached)
		free(t);
	return NULL;
}

v_thread *v_thread_create(v_thread_ctx c, enum v_thread_type type) {
	if (!c.thread_fn)
		return NULL;
	v_thread *t = calloc(1, sizeof(*t));
	t->thread_data = c;
	t->type = type;
#if defined(WINDOWS)
	t->thread_handle = CreateThread(NULL, 0, thread_stub, t, 0, &t->thread_id);
	if (!t->thread_handle) {
		free(t);
		return NULL;
	}
	if (t->type == v_thread_type_detached)
		CloseHandle(t->thread_handle);
	return t;
#else
	pthread_attr_t attribs;
	pthread_attr_init(&attribs);
	int detach_state = 0;
	switch (t->type) {
	case v_thread_type_joinable:
		detach_state = PTHREAD_CREATE_JOINABLE;
		break;
	case v_thread_type_detached:
		detach_state = PTHREAD_CREATE_DETACHED;
		break;
	}
	pthread_attr_setdetachstate(&attribs, detach_state);
	int rc = pthread_create(&t->thread_id, &attribs, thread_stub, t);
	pthread_attr_destroy(&attribs);
	if (rc) {
		free(t);
		return NULL;
	}
	return t;
#endif
}

void *v_thread_wait_and_destroy(v_thread *t) {
	if (t->type == v_thread_type_detached)
		return NULL; // TODO: assert, since this is already a risky move (uaf)
#if defined(WINDOWS)
	WaitForSingleObjectEx(t->thread_handle, INFINITE, FALSE);
	CloseHandle(t->thread_handle);
#else
	int ret = pthread_join(t->thread_id, NULL);
	if (ret) {
		free(t);
		return NULL;
	}
#endif
	void *thread_ret = t->ret;
	free(t);
	return thread_ret;
}

// --- impl v_threadpool (Thread pool + job queue)

/*
	v_threadpool is mostly based on Jon Schember's excellent blog post:
	https://nachtimwald.com/2019/04/12/thread-pool-in-c/
	That code is under the MIT licese:
	Copyright (c) 2019 John Schember <john@nachtimwald.com>

	When I implemented it for c-ray (238f0751f0), my testing revealed a
	race condition in thread_pool_wait(). This is the note I added then:

	"I just added an extra pool->first check to thread_pool_wait()
	since I discovered a race condition in my torture tests for this
	implementation. Basically, sometimes we could blow through a
	call to thread_pool_wait() if we enqueue a small amount of work
	and call thread_pool_wait before threads had a chance to fetch work."

	I also changed some field names to be a bit clearer. I kept mixing them up.
*/

struct v_threadpool_task {
	void (*fn)(void *arg);
	void *arg;
	struct v_threadpool_task *next;
};

struct v_threadpool {
	struct v_threadpool_task *first;
	struct v_threadpool_task *last;
	v_mutex *mutex;
	v_cond *work_available;
	v_cond *work_ongoing;
	size_t active_threads;
	size_t alive_threads;
	char stop_flag;
};

#if defined(__NetBSD__)
	#include <sys/sigtypes.h>
#endif
#if !defined(WINDOWS) && !defined(__APPLE__)
	#include <signal.h>
#endif

static void *v_threadpool_worker(void *arg) {
#if !defined(WINDOWS) && !defined(__APPLE__)
	/* Block all signals, we linux may deliver them to any thread randomly.
	   TODO: Check what macOS, Windows & possibly BSDs do here. */
	sigset_t mask;
	sigfillset(&mask);
	sigprocmask(SIG_SETMASK, &mask, 0);
#endif
	v_threadpool *pool = arg;
	if (!pool)
		return NULL;
	while (1) {
		v_mutex_lock(pool->mutex);
		while (!pool->first && !pool->stop_flag)
			v_cond_wait(pool->work_available, pool->mutex);
		if (pool->stop_flag)
			break;
		struct v_threadpool_task *task = pool->first;
		if (!task->next) {
			pool->first = NULL;
			pool->last = NULL;
		} else {
			pool->first = task->next;
		}
		pool->active_threads++;
		v_mutex_release(pool->mutex);
		if (task) {
			task->fn(task->arg);
			free(task);
		}
		v_mutex_lock(pool->mutex);
		pool->active_threads--;
		if (!pool->stop_flag && pool->active_threads == 0 && !pool->first)
			v_cond_signal(pool->work_ongoing);
		v_mutex_release(pool->mutex);
	}
	pool->alive_threads--;
	v_cond_signal(pool->work_ongoing);
	v_mutex_release(pool->mutex);
	return NULL;
}

v_threadpool *v_threadpool_create(size_t n_threads) {
	if (!n_threads)
		n_threads = v_sys_get_cores() + 1;
	v_threadpool *pool = calloc(1, sizeof(*pool));
	if (!pool)
		goto fail;
	if (!(pool->mutex = v_mutex_create()))
		goto fail;
	if (!(pool->work_available = v_cond_create()))
		goto fail;
	if (!(pool->work_ongoing = v_cond_create()))
		goto fail;

	v_thread_ctx ctx = {
		.thread_fn = v_threadpool_worker,
		.ctx = pool,
	};
	for (size_t i = 0; i < n_threads; ++i) {
		v_thread *success = v_thread_create(ctx, v_thread_type_detached);
		if (!success)
			goto fail;
		pool->alive_threads++;
	}
	return pool;
fail:
	v_threadpool_destroy(pool);
	return NULL;
}

void v_threadpool_destroy(v_threadpool *pool) {
	if (!pool)
		return;
	if (!pool->mutex)
		goto no_mutex;
	if (!pool->work_available)
		goto no_work_available;
	if (!pool->work_ongoing)
		goto no_work_ongoing;

	v_mutex_lock(pool->mutex);
	/* Clear queued tasks */
	struct v_threadpool_task *head = pool->first;
	struct v_threadpool_task *next = NULL;
	while (head) {
		next = head->next;
		free(head);
		head = next;
	}
	/* Signal worker threads to stop */
	pool->stop_flag = 1;
	v_cond_broadcast(pool->work_available);
	v_mutex_release(pool->mutex);

	/* Wait for workers to actually stop */
	v_threadpool_wait(pool);

	/* Finish up */
	v_cond_destroy(pool->work_ongoing);
no_work_ongoing:
	v_cond_destroy(pool->work_available);
no_work_available:
	v_mutex_destroy(pool->mutex);
no_mutex:
	free(pool);
}

int v_threadpool_enqueue(v_threadpool *pool, void (*fn)(void *arg), void *arg) {
	if (!fn)
		return 1;
	if (!pool) {
		fn(arg);
		return 0;
	}
	struct v_threadpool_task *task = malloc(sizeof(*task));
	if (!task)
		return 1;
	*task = (struct v_threadpool_task){
		.fn = fn,
		.arg = arg,
		.next = NULL,
	};
	v_mutex_lock(pool->mutex);
	if (!pool->first) {
		pool->first = task;
		pool->last = pool->first;
	} else {
		pool->last->next = task;
		pool->last = task;
	}
	v_cond_broadcast(pool->work_available);
	v_mutex_release(pool->mutex);
	return 0;
}

void v_threadpool_wait(v_threadpool *pool) {
	if (!pool)
		return;
	v_mutex_lock(pool->mutex);
	while (1) {
		if (pool->first || (!pool->stop_flag && pool->active_threads != 0) || (pool->stop_flag && pool->alive_threads != 0))
			v_cond_wait(pool->work_ongoing, pool->mutex);
		else
			break;
	}
	v_mutex_release(pool->mutex);
}

/*
	This declares a function async_<name>(struct cr_thread_pool *, ...)
	Example:

*/
/*
#define V_ASYNC_JOB(T, v_job_name, v_job_args, ...) \
struct async_##v_job_name##_args v_job_args;\
T do_##v_job_name(struct async_##v_job_name##_args args) __VA_ARGS__ \
void __do_##v_job_name(void *arg) { \
	v_future(T) future = arg; \
	struct async_##v_job_name##_args user_args = *((struct async_##v_job_name##_args *)future->arg); \
	T ret = do_##v_job_name(user_args); \
	mutex_lock(future->mutex); \
	future->result = ret; \
	future->done = 1; \
	free(future->arg); \
	thread_cond_broadcast(&future->sig); \
	mutex_release(future->mutex); \
} \
v_future(T) __async_##v_job_name(struct v_async_ctx *a, struct async_##v_job_name##_args args) { \
	if (!a) return NULL; \
	v_future(T) future = calloc(1, sizeof(*future)); \
	future->arg = calloc(1, sizeof(args));\
	*((struct async_##v_job_name##_args *)future->arg) = args;\
	future->mutex = mutex_create(); \
	thread_cond_init(&future->sig); \
	thread_pool_enqueue(a->pool, __do_##v_job_name, future); \
	return (void *)future; \
}
*/

#ifdef __cplusplus
}
#endif

#endif // V_IMPLEMENTATION

#endif // _V_H_INCLUDED
