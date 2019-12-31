//
//  multiplatform.c
//  C-Ray
//
//  Created by Valtteri on 19/03/2019.
//  Copyright © 2015-2020 Valtteri Koskivuori. All rights reserved.
//


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

void initTerminal() {
	//Disable output buffering
	setbuf(stdout, NULL);
	
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
#endif
}

