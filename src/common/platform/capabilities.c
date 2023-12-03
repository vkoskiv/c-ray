//
//  capabilities.c
//  c-ray
//
//  Created by Valtteri on 29.3.2020.
//  Copyright © 2020-2023 Valtteri Koskivuori. All rights reserved.
//

#include "capabilities.h"

#ifdef __APPLE__
#include <sys/param.h>
#include <sys/sysctl.h>
#elif _WIN32
#include <windows.h>
#elif __linux__ || __COSMOPOLITAN__
#include <unistd.h>
#endif

int sys_get_cores() {
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
#elif __linux__ || __COSMOPOLITAN__
	return (int)sysconf(_SC_NPROCESSORS_ONLN);
#else
	return 1;
#endif
}
