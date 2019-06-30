//
//  timer.h
//  C-ray
//
//  Created by Valtteri Koskivuori on 06/07/2018.
//  Copyright © 2015-2019 Valtteri Koskivuori. All rights reserved.
//


#pragma once

#ifdef WINDOWS
typedef struct timeval {
	long tv_sec;
	long tv_usec;
} TIMEVAL, *PTIMEVAL, *LPTIMEVAL;

int gettimeofday(struct timeval * tp, struct timezone * tzp);
#endif

void startTimer(struct timeval *timer);

long endTimer(struct timeval *timer);

void sleepMSec(int ms);
