//
//  thread.c
//  C-ray
//
//  Created by Valtteri on 29.3.2020.
//  Copyright © 2020 Valtteri Koskivuori. All rights reserved.
//

#include <stdbool.h>
#include <stdint.h>

#include "thread.h"
#include "../logging.h"

void *thread_user_data(void *arg) {
	struct cr_thread *thread = (struct cr_thread *)arg;
	return thread->user_data;
}
// Multiplatform thread stub
#ifdef WINDOWS
DWORD WINAPI thread_stub(LPVOID arg) {
#else
void *thread_stub(void *arg) {
#endif
	return ((struct cr_thread *)arg)->thread_fn(arg);
}

void thread_wait(struct cr_thread *t) {
#ifdef WINDOWS
	WaitForSingleObjectEx(t->thread_handle, INFINITE, FALSE);
#else
	if (pthread_join(t->thread_id, NULL)) {
		logr(warning, "Thread frozen! (This shouldn't happen.)");
	}
#endif
}

int thread_start(struct cr_thread *t) {
#ifdef WINDOWS
	t->thread_handle = CreateThread(NULL, 0, thread_stub, t, 0, &t->thread_id);
	if (t->thread_handle == NULL) return -1;
	return 0;
#else
	pthread_attr_t attribs;
	pthread_attr_init(&attribs);
	pthread_attr_setdetachstate(&attribs, PTHREAD_CREATE_JOINABLE);
	int ret = pthread_create(&t->thread_id, &attribs, thread_stub, t);
	pthread_attr_destroy(&attribs);
	return ret;
#endif
}
