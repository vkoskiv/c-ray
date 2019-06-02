//
//  timer.c
//  C-ray
//
//  Created by Valtteri Koskivuori on 06/07/2018.
//  Copyright © 2015-2019 Valtteri Koskivuori. All rights reserved.
//

#include "../includes.h"
#include "timer.h"

#ifdef WINDOWS
int gettimeofday(struct timeval * tp, struct timezone * tzp) {
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

//Timer funcs
void startTimer(struct timeval *timer) {
	gettimeofday(timer, NULL);
}


/**
 end a given timer and return milliseconds

 @param timer timer to end and measure
 @return milliseconds
 */
unsigned long long endTimer(struct timeval *timer) {
	struct timeval tmr2;
	gettimeofday(&tmr2, NULL);
	return 1000 * (tmr2.tv_sec - timer->tv_sec) + ((tmr2.tv_usec - timer->tv_usec) / 1000);
}

/**
 Sleep for a given amount of milliseconds
 
 @param ms Milliseconds to sleep for
 */
void sleepMSec(int ms) {
#ifdef _WIN32
	Sleep(ms);
#elif __APPLE__
	struct timespec ts;
	ts.tv_sec = ms / 1000;
	ts.tv_nsec = (ms % 1000) * 1000000;
	nanosleep(&ts, NULL);
#elif __linux__
	usleep(ms * 1000);
#endif
}
