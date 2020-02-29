//
//  multiplatform.c
//  C-Ray
//
//  Created by Valtteri on 19/03/2019.
//  Copyright © 2015-2020 Valtteri Koskivuori. All rights reserved.
//

#ifdef WINDOWS
#include <Windows.h>
#else
#include <pthread.h>
#endif

#include "multiplatform.h"

#include "../includes.h"
#include "../utils/logging.h"

//These are for multi-platform physical core detection
#ifdef __APPLE__
#include <sys/param.h>
#include <sys/sysctl.h>
#elif _WIN32
#include <windows.h>
#elif __linux__
#include <unistd.h>
#endif

void showCursor(bool show) {
	show ? fputs("\e[?25h", stdout) : fputs("\e[?25l", stdout);
}

void initTerminal() {
	//If we're on a reasonable (non-windows) terminal, hide the cursor.
#ifndef WINDOWS
	//Disable output buffering
	setbuf(stdout, NULL);
	showCursor(false);
#endif
	
	//Configure Windows terminals to handle color escape codes
#ifdef WINDOWS
	HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
	if (hOut != INVALID_HANDLE_VALUE) {
		DWORD dwMode = 0;
		if (GetConsoleMode(hOut, &dwMode)) {
			dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
			SetConsoleMode(hOut, dwMode);
		}
	}
#endif
}

void restoreTerminal() {
#ifndef WINDOWS
	showCursor(true);
#endif
}

int getSysCores() {
#ifdef __APPLE__
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
#elif _WIN32
	SYSTEM_INFO sysInfo;
	GetSystemInfo(&sysInfo);
	return sysInfo.dwNumberOfProcessors;
#elif __linux__
	return (int)sysconf(_SC_NPROCESSORS_ONLN);
#else
	return 1;
#endif
}

// Multiplatform mutexes

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

// Multiplatform threads

void checkThread(struct crThread *t) {
#ifdef WINDOWS
	WaitForSingleObjectEx(t->thread_handle, INFINITE, FALSE);
#else
	if (pthread_join(t->thread_id, NULL)) {
		logr(warning, "Thread %i frozen.", t);
	}
#endif
}

// Multiplatform thread stub
#ifdef WINDOWS
DWORD WINAPI threadStub(LPVOID arg) {
#else
void *threadStub(void *arg) {
#endif
	struct crThread *thread = (struct crThread*)arg;
	return thread->threadFunc(arg);
}

int spawnThread(struct crThread *t) {
#ifdef WINDOWS
	DWORD threadId; //FIXME: Just pass in &t.thread_id instead like below?
	t->thread_handle = CreateThread(NULL, 0, threadStub, t, 0, &threadId);
	if (t->thread_handle == NULL) {
		return -1;
	}
	t->thread_id = threadId;
	return 0;
#else
	pthread_attr_t attribs;
	pthread_attr_init(&attribs);
	pthread_attr_setdetachstate(&attribs, PTHREAD_CREATE_JOINABLE);
	int ret = pthread_create(&t->thread_id, &attribs, threadStub, t);
	pthread_attr_destroy(&attribs);
	return ret;
#endif
}
