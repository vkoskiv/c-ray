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
	//Disable output buffering
	setbuf(stdout, NULL);
	
	//If we're on a reasonable (non-windows) terminal, hide the cursor.
#ifndef WINDOWS
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
	showCursor(true);
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
