//
//  timer.h
//  C-ray
//
//  Created by Valtteri Koskivuori on 06/07/2018.
//  Copyright © 2018-2020 Valtteri Koskivuori. All rights reserved.
//


#pragma once

#ifdef WINDOWS
typedef struct timeval {
	long tv_sec;
	long tv_usec;
} TIMEVAL, *PTIMEVAL, *LPTIMEVAL;
#endif

void timer_start(struct timeval *timer);

long timer_get_ms(struct timeval timer);

long timer_get_us(struct timeval timer);

void timer_sleep_ms(int ms);
