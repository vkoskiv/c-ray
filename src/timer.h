//
//  timer.h
//  C-Ray
//
//  Created by Valtteri on 06/07/2018.
//  Copyright © 2018 Valtteri Koskivuori. All rights reserved.
//


#pragma once

#ifdef WINDOWS
int gettimeofday(struct timeval * tp, struct timezone * tzp);
#endif

void startTimer(struct timeval *timer);

unsigned long long endTimer(struct timeval *timer);
