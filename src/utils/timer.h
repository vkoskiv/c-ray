//
//  timer.h
//  C-ray
//
//  Created by Valtteri Koskivuori on 06/07/2018.
//  Copyright © 2015-2020 Valtteri Koskivuori. All rights reserved.
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

/**
end a given timer and return milliseconds

@param timer timer to end and measure
@return milliseconds
*/
long getMs(struct timeval timer);

/**
end a given timer and return microseconds

@param timer timer to end and measure
@return microseconds
*/
long getUs(struct timeval timer);

void sleepMSec(int ms);
