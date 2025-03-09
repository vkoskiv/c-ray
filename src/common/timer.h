//
//  timer.h
//  c-ray
//
//  Created by Valtteri Koskivuori on 06/07/2018.
//  Copyright © 2018-2020 Valtteri Koskivuori. All rights reserved.
//


#pragma once

#include "../includes.h"

#ifdef WINDOWS
typedef struct timeval {
	long tv_sec;
	long tv_usec;
} TIMEVAL, *PTIMEVAL, *LPTIMEVAL;
#else
#include <time.h>
#endif

void timer_start(struct timeval *timer);

long timer_get_ms(struct timeval timer);

long timer_get_us(struct timeval timer);

void timer_sleep_ms(int ms);
