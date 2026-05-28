/*
	v.h - My collection of useful bits for writing C programs.

	You may use this, but I can't guarantee you'll find it useful.

	Remember to #define V_IMPLEMENTATION before #including v.h in
	your code. If you include v.h in multiple files, place that define
	before exactly one of your #includes. Shouldn't matter which one, as
	long as you don't define V_IMPLEMENTATION more than once in your project.

	Copyright (c) 2025-2026, Valtteri Koskivuori
	SPDX-License-Identifier: MIT

	Feature list. Search for:
	- `--- decl <name>` to get to the API
	- `--- impl <name>` to get to the implementation

	v.h has partial support for nostdlib/freestanding environments, which is
	determined if you defined `V_NOSTDLIB` before #including v.h.
	The following restrictions apply in this case:
	- Features/subfeatures marked with `+` are disabled entirely.
	- Features marked with `*` are enabled if you define the listed macros.

	- v_ilist: Intrusive doubly-linked list
	- v_hash: (Fowler-Noll-Vo hash function)
	- v_ht: TODO: Hash-table (port from c-ray)
	- v_cbuf: TODO: Circular buffer (port from refmon)
	- v_str: TODO UTF-8 String library
	- v_tok: String tokenizer
		+ v_tok_to_arr() requires HAVE_V_ARR on v_arr
		+ v_tok_dump() requires defined(V_HAVE_STDLIB)
	* v_timer: Simple timer gadget
		- Must define: v_nostd_gettimeofday(struct timeval *tv, void *tz)
		- Must define: v_nostd_sleep_ms(int ms)
	* v_ma: Arena allocator
		- Must define: v_nostd_memcpy()
		- Must define: v_nostd_memset()
		+ v_ma_from_heap() requires defined(V_HAVE_STDLIB)
		+ v_ma_destroy() requires defined(V_HAVE_STDLIB)
	+ v_mp: Memory pool allocator
	+ v_arr: Dynamic arrays (TODO: implement v_mem & maybe bring this to *?)
	+ v_sys: Query system information       (*nix, win32)
	+ v_mod: Dynamic module support         (*nix, win32)
	+ v_dir: TODO: Directory utilities (port from refmon)
	+ v_sync: Sync primitives (mutex, rwlock, condition vars), (pthreads, win32)
	+ v_thread: Threading abstraction & thread pool (pthreads, win32)
	+ v_threadpool: Thread pool + job queue
 */

#ifndef _V_H_INCLUDED
#define _V_H_INCLUDED 1

#if defined(__linux__)
	#define V_PLATFORM linux
#elif defined(__APPLE__)
	#define V_PLATFORM apple
#elif defined(__NetBSD__)
	#define V_PLATFORM netbsd
#elif defined(WINDOWS) || defined(_WIN32)
	#define V_PLATFORM windows
#elif defined(__COSMOPOLITAN__)
	#define V_PLATFORM cosmo
#else
	#warning "Unknown platform, assuming linux."
	#define V_PLATFORM linux
#endif

/* Platform-specific tweaks */
#if V_PLATFORM == netbsd && !defined(_NETBSD_SOURCE)
	#define _NETBSD_SOURCE /* stack_t */
#endif

#if !defined(V_NOSTDLIB)
	#define V_HAVE_STDLIB
#endif

/* v.h depends on C99 */
#if !defined(__STDC_VERSION__) || __STDC_VERSION__ < 199901L
	#error "The v.h library requires C99 or later."
#endif

/* v.h depends on pthread_rwlock_t and other POSIX goodies */
#ifndef _POSIX_C_SOURCE
	#define _POSIX_C_SOURCE 200112L
#endif

// TODO: Unified V_PLATFORM == <plat> macro

// --- Feature tests & enable ---

#if defined(V_HAVE_STDLIB)
	#define v__gettimeofday(tv, tz) gettimeofday((tv), (tz))
	// v__sleep_ms(ms) omitted, platform tests in impl FIXME: Clean that up
	#define v__memcpy(dst, src, n) memcpy((dst), (src), (n))
	#define v__memset(s, c, n) memset((s), (c), (n))
	#define v__abort() abort()
#else
	#if defined(v_nostd_gettimeofday)
		struct timeval { long tv_sec; long tv_usec; };
		#define v__gettimeofday(tv, tz) v_nostd_gettimeofday((tv), (tz))
	#endif

	#if defined(v_nostd_sleep_ms)
		#define v__sleep_ms(ms) v_nostd_sleep_ms((ms))
	#endif

	#if defined(v_nostd_memcpy)
		#define v__memcpy(dst, src, n) v_nostd_memcpy(dst, src, n)
	#endif

	#if defined(v_nostd_memset)
		#define v__memset(s, c, n) v_nostd_memset((s), (c), (n))
	#endif

	#if defined(v_nostd_abort)
		#define v__abort() v_nostd_abort()
	#endif
#endif

#define HAVE_V_ILIST
#define HAVE_V_HASH
#define HAVE_V_HT
#define HAVE_V_CBUF
#define HAVE_V_STR
#define HAVE_V_TOK
#if defined(v__sleep_ms) || defined(v__gettimeofday)
	#define HAVE_V_TIMER
#endif
#if defined(v__memcpy) && defined(v__memset) && defined(v__abort)
	#define HAVE_V_MA
#endif

#if defined(V_HAVE_STDLIB)
	#define HAVE_V_MP
	#define HAVE_V_ARR
	#define HAVE_V_SYS
	#define HAVE_V_MOD
	#define HAVE_V_DIR
	#define HAVE_V_SYNC
	#define HAVE_V_THREAD
	// TODO: threadpool depends on v_sync, v_thread and stdlib malloc.
	// consider expressing deps with HAVE_V_* once v_mem wraps malloc.
	#define HAVE_V_THREADPOOL
#endif

// TODO: Consolidate all includes in this one block
#include <stddef.h>
#include <stdint.h>

#if defined(V_HAVE_STDLIB)
#include <string.h> // for memcpy() in v_arr_add_n()
#include <stdlib.h> // We depend on stdlib malloc/free/realloc for now
#include <sys/time.h> // gettimeofday(), FIXME: Check bsd/macOS/etc.
#endif

// --- begin declarations ---

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

#ifdef __cplusplus
extern "C" {
#endif

// --- decl v_ilist (Intrusive doubly-linked list)

#ifdef HAVE_V_ILIST
	struct v_ilist {
		struct v_ilist *prev;
		struct v_ilist *next;
	};
	typedef struct v_ilist v_ilist;

	#define V_ILIST_INIT(name) { .prev = &(name), .next = &(name) }
	#define V_ILIST(name) v_ilist name = V_ILIST_INIT(name)
	// TODO: examples:
	// - queue demo with prepend
	// - stack demo with append
	// TODO: tests
	#define v_ilist_get(ptr, type, member) \
		v_container_of(ptr, type, member)
	#define v_ilist_get_last(ptr, type, member) \
		v_container_of((ptr)->prev, type, member)
	#define v_ilist_get_first(ptr, type, member) \
		v_container_of((ptr)->next, type, member)
	#define v_ilist_for_each(node, head) \
		for (node = (head)->next; \
		!v_ilist_is_head((head), node); \
		node = node->next)
	#define v_ilist_for_each_safe(node, temp, head) \
		for (node = (head)->next, temp = node->next; \
		!v_ilist_is_head((head), node); \
		node = temp, temp = node->next)
	#define v_ilist_for_each_continue(node, head) \
		for (node = node->next; \
		!v_ilist_is_head((head), node); \
		node = node->next)
	static inline void v_ilist_init(v_ilist *head) {
		head->prev = head;
		head->next = head;
	}
	#define V__ILIST_LINK \
		prev->next = node; \
		node->prev = prev; \
		next->prev = node; \
		node->next = next;
	static inline v_ilist *v_ilist_prepend(v_ilist *node, v_ilist *next) {
		v_ilist *prev = next->prev;
		V__ILIST_LINK
		return node;
	}
	static inline v_ilist *v_ilist_append(v_ilist *node, v_ilist *prev) {
		v_ilist *next = prev->next;
		V__ILIST_LINK
		return node;
	}
	static inline void v_ilist_remove(v_ilist *node) {
		if (!node)
			return;
		v_ilist *prev = node->prev;
		v_ilist *next = node->next;
		prev->next = next;
		next->prev = prev;
		v_ilist_init(node);
	}
	static inline void v_ilist_replace(v_ilist *old, v_ilist *new) {
		new->next = old->next;
		new->next->prev = new;
		new->prev = old->prev;
		new->prev->next = new;
	}
	static inline void v_ilist_swap(v_ilist *a, v_ilist *b) {
		v_ilist *spot = b->prev;
		v_ilist_remove(b);
		v_ilist_replace(a, b);
		if (spot == a)
			spot = b;
		v_ilist_append(a, spot);
	}
	static inline int v_ilist_is_first(const v_ilist *head,
	                                   const v_ilist *node) {
		return node->prev == head;
	}
	static inline int v_ilist_is_last(const v_ilist *head,
	                                  const v_ilist *node) {
		return node->next == head;
	}
	static inline int v_ilist_is_head(const v_ilist *head,
	                                  const v_ilist *node) {
		return node == head;
	}
	static inline int v_ilist_is_empty(const v_ilist *head) {
		return head->next == head;
	}
	static inline void v_ilist_destroy(v_ilist *head,
	                                   void (*dtor)(void *elem)) {
		v_ilist *pos, *temp;
		v_ilist_for_each_safe(pos, temp, head) {
			v_ilist_remove(pos);
			if (dtor)
				dtor(pos);
		}
	}
	static inline size_t v_ilist_count(const v_ilist *head) {
		v_ilist *pos;
		size_t elems = 0;
		v_ilist_for_each(pos, head)
			elems++;
		return elems;
	}
#endif /* HAVE_V_ILIST */

// --- decl v_hash (Fowler-Noll-Vo hash function)

#ifdef HAVE_V_HASH
	typedef uint32_t v_hash;

	#define v_hash_init() v_hash_bytes(NULL, NULL, 0);
	#define v_hash(H, I) v_hash_bytes((H), &(I), sizeof((I)))
	#define v_hash_cstr(H, S) v_hash_bytes((H), (S), strlen((S)))
	v_hash v_hash_bytes(v_hash *prev, const void *data, size_t size);
#endif /* HAVE_V_HASH */

// --- decl v_ht (Hash table w/ FNV)(c-ray)

#ifdef HAVE_V_HT
	// TODO
#endif /* HAVE_V_HT */

// --- decl v_cbuf (Circular buffers for running averages) (refmon)

#ifdef HAVE_V_CBUF
	// TODO
#endif /* HAVE_V_CBUF */

// --- decl v_str (UTF-8 checked strings)

#ifdef HAVE_V_STR
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
#endif /* HAVE_V_STR */

// --- decl v_tok (String tokenizer)

#ifdef HAVE_V_TOK
	typedef struct {
		const char *beg;
		const char *end;
		char sep;
	} v_tok;
	#define v_tok(str, c) (v_tok){ .beg = (str), .end = (str) + strlen((str)), .sep = (c) }

	#define V_TOK_FMT "%.*s"
	#define v_tok_fmt(tk) (int)v_tok_len(tk), tk.beg
#if defined(V_HAVE_STDLIB)
	#define v_tok_dump(tk) (v_tok_empty(tk) ? 0 : fprintf(stderr, "%s(%lu): '%.*s'\n", #tk, v_tok_count(tk), v_tok_fmt(tk)));
#endif
	size_t v_tok_len(v_tok t);
	int v_tok_empty(v_tok t);
	v_tok v_tok_peek(v_tok t);
	v_tok v_tok_consume(v_tok *t);
	char v_tok_peek_c(v_tok t);
	char v_tok_consume_c(v_tok *t);
	size_t v_tok_count(v_tok t);
	int v_tok_eq(v_tok tk, const char *str);
#if defined(HAVE_V_ARR)
	v_tok *v_tok_to_arr(v_tok toks);
#endif
#endif /* HAVE_V_TOK */

// --- decl v_timer (Simple timers)

#ifdef HAVE_V_TIMER
// TODO: Consider moving up to feature test & enable?
#if defined(WINDOWS)
	typedef struct timeval {
		long tv_sec;
		long tv_usec;
	} TIMEVAL, *PTIMEVAL, *LPTIMEVAL;
#endif

	typedef struct timeval v_timer;

#if defined(v__gettimeofday)
	v_timer v_timer_start(void);
	long v_timer_get_ms(v_timer t);
	long v_timer_get_lap_ms(v_timer *t);
	long v_timer_get_us(v_timer t);
	long v_timer_get_lap_us(v_timer *t);
#endif
#if defined(V_HAVE_STDLIB) || defined(v__sleep_ms)
	void v_timer_sleep_ms(int ms);
#endif
#endif /* HAVE_V_TIMER */

// --- decl v_ma (Arena allocator)

#ifdef HAVE_V_MA
/*
	Largely inspired by:
	https://nullprogram.com/blog/2023/09/27/
*/

struct v_ma {
	uint8_t *alloc;
	uint8_t *beg;
	uint8_t *end;
	void *jmp_buf[5]; // x86_64: rbp, rip, rsp, 0, 0
	int flags;
};
typedef struct v_ma v_ma;

void *_v_ma_alloc(v_ma *a, ptrdiff_t size, ptrdiff_t align, ptrdiff_t count, int flags, void *data);
#define _v_newx(a, b, c, d, e, ...) e
#define _v_new2(arena, type)               (type *)_v_ma_alloc(arena, sizeof(type), v_alignof(type),     1,     0, NULL)
#define _v_new3(arena, type, count)        (type *)_v_ma_alloc(arena, sizeof(type), v_alignof(type), count,     0, NULL)
#define _v_new4(arena, type, count, flags) (type *)_v_ma_alloc(arena, sizeof(type), v_alignof(type), count, flags, NULL)


	#define V_MA_NOZERO (1 << 0)
	#define V_MA_SOFTFAIL (1 << 1)
	#define V_MA_DO_LONGJMP (1 << 2)

	#define V_KiB 1024ull
	#define V_MiB (V_KiB * V_KiB)
	#define V_GiB (V_KiB * V_MiB)
	#define V_TiB (V_KiB * V_GiB)

	v_ma v_ma_from_buf(uint8_t *buf, ptrdiff_t capacity);
	v_ma v_ma_from_ma(v_ma *a, ptrdiff_t capacity);
	#define v_ma_from_arr(array) v_ma_from_buf(array, sizeof(array))
#if defined(V_HAVE_STDLIB)
	v_ma v_ma_from_heap(ptrdiff_t capacity);
	void v_ma_destroy(v_ma *a);
#endif

	// Very useful, but do not use if you received a pointer to v_ma
	// e.g. v_ma_on_oom((*arena_arg)) { ... } is a *bad* idea, and *will* blow up your stack.
	#define v_ma_on_oom(arena) if (arena.flags |= V_MA_DO_LONGJMP, __builtin_setjmp(arena.jmp_buf))

	// TODO: Maybe have these act on a generic allocator interface that uses e.g. v_ma/etc behind the scenes?
	#define v_new(...) _v_newx(__VA_ARGS__, _v_new4, _v_new3, _v_new2)(__VA_ARGS__)
	#define v_put(arena, type, ...) _v_ma_alloc(arena, sizeof(type), v_alignof(type), 1, 0, &(type)__VA_ARGS__)

#endif /* HAVE_V_MA */

// --- decl v_mp (Memory pool allocator)

#ifdef HAVE_V_MP
	struct v_mp;
	typedef struct v_mp v_mp;

	v_mp *v_mp_create(size_t initial_size);
	void *v_mp_alloc(v_mp **head, size_t size);
	void v_mp_destroy(v_mp *head);
#endif /* HAVE_V_MP */

// --- decl v_arr (Dynamic arrays)

#ifdef HAVE_V_ARR
// TODO: Maybe have a flexible struct member & use v_offsetof instead of that array indexing trick
// - Benefits? Flaws?
// TODO: Alignment?
// - I guess it's enough to pad v_arr to ensure it aligns to v_max_align_t?
// TODO: Rename to v__arr_head
struct v_arr {
	size_t n;
	size_t cap;
	size_t elem_size;
	size_t (*grow_fn)(size_t cap, size_t elem_size);
	// v_mem *allocator; <- Used if not NULL, and this type casts to e.g. v_ma, v_stdalloc, v_mempool, etc.
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
void *v__arr_trim(void *a);
void *v__arr_copy(const void *const a);
void v__arr_free(void *a);
#define v__arr_head(A) ((struct v_arr *)(A) - 1)
#define v__arr_grow(A, N) ((A) = v__arr_do_grow((A), sizeof(*A), (N)))
// TODO: Maybe expose v__arr_ensure() in something like v_arr_new(T, N)?
#define v__arr_ensure(A, N) \
	(((!A) || v__arr_head(A)->n + N > v__arr_head(A)->cap) ? (v__arr_grow(A, N), 0) : 0)


	// TODO: It's quite easy to forget & when using v_arr_trim().
	// Use the v_arr_free() pattern and directly assign result of v__arr_trim() to A
	// TODO: v_arr_free() isn't consistent with *_destroy() elsewhere, rename?
	// FIXME: I already confused v_arr_len with v_arr_cap when writing ringbuf experiment.
	#define v_arr_cap(A)             ((A) ? v__arr_head(A)->cap : 0)
	#define v_arr_len(A)             ((A) ? v__arr_head(A)->n : 0)
	#define v_arr_add(A, ...)        (v__arr_ensure((A), 1), (A)[v__arr_head(A)->n] = (__VA_ARGS__), v__arr_head(A)->n++)
	#define v_arr_append(A)          (v__arr_ensure((A), 1), &(A)[v__arr_head(A)->n++])
	#define v_arr_add_n(A, items, N) (v__arr_ensure((A), N), memcpy((A) + v__arr_head((A))->n, items, N * sizeof(items[0])), v__arr_head((A))->n += N)
	#define v_arr_trim(A)            ((A) ? ((A) = v__arr_trim((A)), 0) : 0)
	#define v_arr_copy(A)            ((A) ? v__arr_copy((A)) : NULL)
	#define v_arr_free(A)            (((A) ? (v__arr_free((A)), 0) : 0), (A) = NULL)
	#define v_arr_join(A, B)         ((A) && (B) ? (v_arr_add_n((A), (B), v_arr_len((B))), v_arr_free((B)), (A)) : NULL)
	#define v_arr_set_grow_fn(A, fn) (v__arr_ensure((A), 0), v__arr_head(A)->grow_fn = fn)
	#define v_arr_set_elem_free(A, fn) (v__arr_ensure((A), 0), v__arr_head(A)->elem_free = fn)
#endif /* HAVE_V_ARR */

// --- decl v_sys (System capabilities)

#ifdef HAVE_V_SYS
	int v_sys_get_cores(void);
#endif /* HAVE_V_SYS */

// --- decl v_mod (Runtime module loading)

#ifdef HAVE_V_MOD
	typedef void * v_mod;
	v_mod v_mod_load(const char *filename);
	void *v_mod_sym(v_mod handle, const char *name);
	char *v_mod_error(void);
	int v_mod_close(v_mod handle);
#endif /* HAVE_V_MOD */

// --- decl v_dir (Directory iteration goodies) (refmon)

#ifdef HAVE_V_DIR
	// TODO
#endif /* HAVE_V_DIR */

// --- decl v_mem (Composable memory allocators)

#ifdef HAVE_V_MEM
/*
	TODO
	- Stash allocation context before returned ptr, like in v_arr
	- that ctx tells v_mem_free() if it should free(), munmap() or do nothing
	  if it was a stack allocation
	- Figure out a strategy for cases where a feature requires realloc(), but
	  the provided v_mem context doesn't support it (stack/mmap)

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
	// 	.ctx = v_ma,
	// 	.alloc = v_ma_alloc,
	// 	.cfree = NULL,
	// };
	// And then that could get passed into things, and defaults to NULL if no allocator
	// struct v_mutex *v_mutex_create(struct v_alloc *a) {
	// 	struct v_mutex *m = a ? a->zalloc(1, sizeof(*m)) : v_alloc_stdlib.zalloc(1, sizeof(*m));
	// 	// etc, etc
	// }

*/
#endif /* HAVE_V_MEM */

// --- decl v_sync (Sync primitives (mutex, rwlock, condition variables), pthreads & win32)

#ifdef HAVE_V_SYNC
	struct timespec;

	// FIXME: I don't like not having a static initializer like pthreads'
	// PTHREAD_MUTEX_INITIALIZER, and having to always allocate these & pair
	// with v_mutex_destroy().
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
#endif /* HAVE_V_SYNC */

// --- decl v_thread (Threading abstraction, pthreads & win32)

#ifdef HAVE_V_THREAD
	struct v_thread;
	typedef struct v_thread v_thread;

	enum v_thread_type {
		v_thread_type_joinable = 0,
		v_thread_type_detached,
	};

	/*
		NOTE: Return value of v_thread_spawn() is freed automatically when
		type == v_thread_type_detached, and should only be used to check
		for errors (NULL), and not stored. Passing it to v_thread_wait() is
		considered undefined.
		In the case type == v_thread_type_joinable, the required call to
		v_thread_{wait,stop}() calls free() on v_thread *.
		TODO: Optional allocator
	*/
	v_thread *v_thread_spawn(void *(*proc)(void *), void *ctx, enum v_thread_type type);
	void *v_thread_wait(v_thread *); // polite, try this first
	void *v_thread_stop(v_thread *); // rude, design a better event loop in your thread instead
#endif /* HAVE_V_THREAD */

// --- decl v_threadpool (Thread pool + job queue)

#ifdef HAVE_V_THREADPOOL
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
#endif /* HAVE_V_THREADPOOL */

#ifdef __cplusplus
}
#endif

// --- end declarations ---

#ifdef V_IMPLEMENTATION
#undef V_IMPLEMENTATION

// --- begin implementations ---

#ifdef __cplusplus
extern "C" {
#endif

// --- impl v_ilist (Intrusive doubly-linked list)

#ifdef HAVE_V_ILIST
	// This space intentionally left blank.
#endif /* HAVE_V_ILIST */

// --- impl v_hash (Fowler-Noll-Vo hash function)

#ifdef HAVE_V_HASH
#define V__FNV_OFFSET UINT32_C(0x811C9DC5)
#define V__FNV_PRIME  UINT32_C(0x01000193)

static v_hash v__hash_combine(v_hash h, uint8_t b) {
	return (h ^ b) * V__FNV_PRIME;
}

v_hash v_hash_bytes(v_hash *prev, const void *data, size_t size) {
	v_hash h = prev ? *prev : V__FNV_OFFSET;
	for (size_t i = 0; i < size; ++i)
		h = v__hash_combine(h, ((uint8_t *)data)[i]);
	if (prev)
		*prev = h;
	return h;
}
#endif /* HAVE_V_HASH */

// --- impl v_ht (Hash table w/ FNV)(c-ray)

#ifdef HAVE_V_HT
	// TODO
#endif /* HAVE_V_HT */

// --- impl v_cbuf (Circular buffers for running averages) (refmon)

#ifdef HAVE_V_CBUF
	// TODO
#endif /* HAVE_V_CBUF */

// --- impl v_str (UTF-8 checked strings)

#ifdef HAVE_V_STR
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
#endif /* HAVE_V_STR */

// --- impl v_tok (String tokenizer)

#ifdef HAVE_V_TOK
size_t v_tok_len(v_tok t) {
	return (uintptr_t)t.end - (uintptr_t)t.beg;
}

int v_tok_empty(v_tok t) {
	return (uintptr_t)t.beg >= (uintptr_t)t.end;
}

v_tok v_tok_peek(v_tok t) {
	const char *end = t.end;
	while (t.beg < t.end && *t.beg == t.sep)
		t.beg++;
	t.end = t.beg;
	while (t.end < end && *t.end != t.sep)
		t.end++;
	return t;
}

v_tok v_tok_consume(v_tok *t) {
	v_tok ret = *t;
	while (ret.beg < ret.end && *ret.beg == t->sep)
		ret.beg++;
	ret.end = ret.beg;
	while (ret.end < t->end && *ret.end != t->sep)
		ret.end++;
	t->beg = ret.end + 1;
	return ret;
}

char v_tok_peek_c(v_tok t) {
	return *t.beg;
}

char v_tok_consume_c(v_tok *t) {
	return *t->beg++;
}

size_t v_tok_count(v_tok t) {
	size_t n = 0;
	while ((n++, v_tok_consume(&t), !v_tok_empty(t)));
	return n;
}

int v_tok_eq(v_tok tk, const char *str) {
	for (size_t i = 0; i < v_tok_len(tk); ++i)
		if (tk.beg[i] != str[i])
			return 0;
	return 1;
}

#if defined(HAVE_V_ARR)
v_tok *v_tok_to_arr(v_tok t) {
	v_tok *arr = { 0 };
	v_tok next = { 0 };
	while ((next = v_tok_consume(&t), !v_tok_empty(t)))
		v_arr_add(arr, next);
	return arr;
}
#endif
#endif /* HAVE_V_TOK */

// --- impl v_timer (Simple timers)

#ifdef HAVE_V_TIMER
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

#if defined(v__gettimeofday)

v_timer v_timer_start(void) {
	v_timer timer = { 0 };
	v__gettimeofday(&timer, NULL);
	return timer;
}

long v_timer_get_ms(v_timer t) {
	v_timer now;
	v__gettimeofday(&now, NULL);
	return 1000 * (now.tv_sec - t.tv_sec) + ((now.tv_usec - t.tv_usec) / 1000);
}

long v_timer_get_lap_ms(v_timer *t) {
	v_timer now;
	v__gettimeofday(&now, NULL);
	long ms = 1000 * (now.tv_sec - t->tv_sec) + ((now.tv_usec - t->tv_usec) / 1000);
	v__gettimeofday(t, NULL);
	return ms;
}

long v_timer_get_us(v_timer t) {
	v_timer now;
	v__gettimeofday(&now, NULL);
	return ((now.tv_sec - t.tv_sec) * 1000000) + (now.tv_usec - t.tv_usec);
}

long v_timer_get_lap_us(v_timer *t) {
	v_timer now;
	v__gettimeofday(&now, NULL);
	long us = ((now.tv_sec - t->tv_sec) * 1000000) + (now.tv_usec - t->tv_usec);
	v__gettimeofday(t, NULL);
	return us;
}

#endif /* #if defined(v__gettimeofday) */

#if defined(V_HAVE_STDLIB) && defined(__linux__)
	#define _BSD_SOURCE
	// #include <unistd.h>
	#include "/usr/include/unistd.h"
	#include <errno.h>
	#include <time.h>
#endif

#if defined(V_HAVE_STDLIB) && defined(__APPLE__)
	#include <time.h>
#endif

#if defined(V_HAVE_STDLIB) || defined(v__sleep_ms)
void v_timer_sleep_ms(int ms) {
#if !defined(V_HAVE_STDLIB)
	v__sleep_ms(ms);
#elif defined(WINDOWS)
	Sleep(ms);
#elif defined(__APPLE__)
	struct timespec ts;
	ts.tv_sec = ms / 1000;
	ts.tv_nsec = (ms % 1000) * 1000000;
	nanosleep(&ts, NULL);
#elif defined (__linux__)
	struct timespec ts = { .tv_sec = ms / 1000, .tv_nsec = (ms % 1000) * 1000 * 1000 };
	struct timespec rem;
	while (nanosleep(&ts, &rem) == EINTR) {
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
#endif /* #if defined(V_HAVE_STDLIB) || defined(v__sleep_ms) */

#endif /* HAVE_V_TIMER */

// --- impl v_ma (Arena allocator)

#ifdef HAVE_V_MA

void *_v_ma_alloc(v_ma *a, ptrdiff_t size, ptrdiff_t align, ptrdiff_t count, int flags, void *data) {
	ptrdiff_t padding = -(uintptr_t)a->beg & (align - 1);
	ptrdiff_t available = a->end - a->beg - padding;
	// TODO: Original nullprogram impl had this first check
	// as available < 0, which seems wrong. Maybe look into that?
	if (available <= 0 || count > available / size) {
		if (a->flags & V_MA_DO_LONGJMP)
			// __builtin_longjmp(a->jmp_buf, 1);
			v__abort();
		else if ((flags & V_MA_SOFTFAIL) || (a->flags & V_MA_SOFTFAIL))
			return NULL;
		else
			v__abort();
	}
	void *p = a->beg + padding;
	ptrdiff_t bytes = count * size;
	a->beg += bytes + padding;
	if (data)
		return v__memcpy(p, data, size);
	return ((flags & V_MA_NOZERO) || (a->flags & V_MA_NOZERO)) ?
	    p :
	    v__memset(p, 0, bytes);
}

v_ma v_ma_from_buf(uint8_t *buf, ptrdiff_t capacity) {
	return (v_ma){
		.beg = buf,
		.end = buf ? buf + capacity : 0
	};
}

v_ma v_ma_from_ma(v_ma *a, ptrdiff_t capacity) {
	uint8_t *buf = v_new(a, uint8_t, capacity);
	v_ma new = v_ma_from_buf(buf, capacity);
	new.flags = a->flags;
	return new;
}

// TODO: have internal logic to e.g. use mmap()/VirtualAlloc()
// TODO: Consider deleting these two entirely, maybe substituting with
// helpers to easily e.g. mmap an arena.
#if defined(V_HAVE_STDLIB)
v_ma v_ma_from_heap(ptrdiff_t capacity) {
	uint8_t *buf = malloc(capacity);
	v_ma mem = v_ma_from_buf(buf, capacity);
	mem.alloc = buf;
	return mem;
}

void v_ma_destroy(v_ma *a) {
	if (!a)
		return;
	a->beg = a->end = 0;
	if (a->alloc) {
		uint8_t *p = a->alloc;
		a->alloc = NULL;
		free(p);
	}
}
#endif /* defined(V_HAVE_STDLIB) */

#endif /* HAVE_V_MA */

// --- impl v_mp (Memory pool allocator)

#ifdef HAVE_V_MP
struct v_mp {
	size_t n;
	size_t cap;
	struct v_mp *prev;
	v_max_align_t data[];
};

static v_mp *v__mp_create(v_mp *prev, size_t initial_size) {
	v_mp *block = calloc(1, sizeof(*block) + initial_size);
	block->cap = initial_size;
	block->n = 0;
	block->prev = prev;
	return block;
}

v_mp *v_mp_create(size_t initial_size) {
	return v__mp_create(NULL, initial_size);
}

void *v_mp_alloc(v_mp **head, size_t size) {
	if (!size)
		return NULL;
	// Round up for alignment
	size += sizeof(v_max_align_t) - (size % sizeof(v_max_align_t));

	if ((*head)->n + size > (*head)->cap) {
		size_t next_size = (*head)->cap > size ? (*head)->cap : size;
		*head = v__mp_create(*head, next_size);
	}
	void *ptr = (char *)(*head)->data + (*head)->n;
	(*head)->n += size;
	return ptr;
}

void v_mp_destroy(v_mp *head) {
	while (head) {
		v_mp *prev = head->prev;
		free(head);
		head = prev;
	}
}
#endif /* HAVE_V_MP */

// --- impl v_arr (Dynamic arrays)

#ifdef HAVE_V_ARR
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
	v__memcpy(copy, a, v_arr_len(a) * v__arr_head(a)->elem_size);
	return copy;
}

void v__arr_free(void *a) {
	struct v_arr *arr = v__arr_head(a);
	for (size_t i = 0; i < arr->n; ++i) {
		if (arr->elem_free)
			arr->elem_free(((char *)a) + (arr->elem_size * i));
	}
	// TODO: if (arr->allocator) ...
	free(arr);
}
#endif /* HAVE_V_ARR */

// --- impl v_sys (System capabilities)

#ifdef HAVE_V_SYS
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
#endif /* HAVE_V_SYS */

// --- impl v_mod (Runtime library loading) (c-ray) ---

#ifdef HAVE_V_MOD
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
#endif /* HAVE_V_MOD */

// --- impl v_dir (Directory iteration goodies) (refmon)

#ifdef HAVE_V_DIR
	// TODO
#endif /* HAVE_V_DIR */

// --- impl v_sync (Sync primitives (mutex, rwlock, condition variables), pthreads & win32)

#ifdef HAVE_V_SYNC
#if defined(WINDOWS)
	// TODO: Consider WIN32_LEAN_AND_MEAN
	// TODO: Set up regular testing on Windows
	#include <Windows.h>
	#include <time.h>
#else
	#include <pthread.h>
#endif

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
#endif /* HAVE_V_SYNC */

// --- impl v_thread (Threading abstraction, pthreads & win32)

#ifdef HAVE_V_THREAD
struct v_thread {
#if defined(WINDOWS)
	HANDLE thread_handle;
	DWORD thread_id;
#else
	pthread_t thread_id;
#endif
	enum v_thread_type type;
	void *(*thread_fn)(void *); // The function to run in this thread
	void *ctx;                  // Thread context, this gets passed to thread_fn
	void *ret;
};

#if defined(WINDOWS)
static DWORD WINAPI thread_stub(LPVOID arg) {
#else
static void *thread_stub(void *arg) {
#endif
	v_thread *t = (struct v_thread *)arg;
	t->ret = t->thread_fn(t->ctx);
	if (t->type == v_thread_type_detached)
		free(t);
	return NULL;
}

v_thread *v_thread_spawn(void *(*proc)(void *), void *ctx, enum v_thread_type type) {
	if (!proc)
		return NULL;
	v_thread *t = calloc(1, sizeof(*t));
	t->thread_fn = proc;
	t->ctx = ctx;
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

void *v_thread_wait(v_thread *t) {
	if (t->type == v_thread_type_detached)
		return NULL; // TODO: assert, since this is already a risky move (uaf)
#if defined(WINDOWS)
	WaitForSingleObjectEx(t->thread_handle, INFINITE, FALSE);
	CloseHandle(t->thread_handle);
#else
	void *status = NULL;
	int ret = pthread_join(t->thread_id, &status);
	if (ret) {
		free(t);
		return NULL;
	}
	if (status == PTHREAD_CANCELED) {
		free(t);
		return NULL;
	}
#endif
	void *thread_ret = t->ret;
	free(t);
	return thread_ret;
}

void *v_thread_stop(v_thread *t) {
	if (t->type == v_thread_type_detached)
		return NULL;
#if defined(WINDOWS)

#else
	// FIXME: Figure out a good fatal error strategy.
	// I don't like the idea of littering abort() calls in
	// these branches. Maybe an opt-in V_VERIFY_STRICT define?
	int ret = pthread_cancel(t->thread_id);
	if (ret) {
		free(t);
		return NULL;
	}

	void *status = NULL;
	ret = pthread_join(t->thread_id, &status);
	if (ret) {
		free(t);
		return NULL;
	}
	if (status != PTHREAD_CANCELED) {
		free(t);
		return NULL;
	}
#endif
	void *thread_ret = t->ret;
	free(t);
	return thread_ret;
}
#endif /* HAVE_V_THREAD */

// --- impl v_threadpool (Thread pool + job queue)

#ifdef HAVE_V_THREADPOOL
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

	for (size_t i = 0; i < n_threads; ++i) {
		v_thread *success = v_thread_spawn(v_threadpool_worker, pool, v_thread_type_detached);
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
#endif /* HAVE_V_THREADPOOL */

#ifdef __cplusplus
}
#endif

#endif // V_IMPLEMENTATION

// --- end implementations ---

#endif // _V_H_INCLUDED
