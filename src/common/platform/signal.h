//
//  signal.h
//  c-ray
//
//  Created by Valtteri on 7.4.2020.
//  Copyright © 2020 Valtteri Koskivuori. All rights reserved.
//

#pragma once

#if !defined(WINDOWS) && !defined(__APPLE__)
	#include <signal.h>
#endif

// By default, signals may be delivered to any running thread
// so block them in background threads.
// Thanks to jart for this TIL, I saw it in cosmo turfwar.c
static inline void block_signals(void) {
#if !defined(WINDOWS) && !defined(__APPLE__)
	sigset_t mask;
	sigfillset(&mask);
	sigprocmask(SIG_SETMASK, &mask, 0);
#endif
}
