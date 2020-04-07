//
//  signal.c
//  C-ray
//
//  Created by Valtteri on 7.4.2020.
//  Copyright © 2020 Valtteri Koskivuori. All rights reserved.
//

#include "signal.h"
#include <signal.h>
#include "../logging.h"

//Signal handling
void (*signal(int signo, void (*func )(int)))(int);
typedef void sigfunc(int);
sigfunc *signal(int, sigfunc*);

int registerHandler(enum sigtype type, void (*handler)(int)) {
	int sig = 0;
	switch (type) {
		case sigint:
			sig = SIGINT;
			break;
		case sigabrt:
			sig = SIGABRT;
		default:
			sig = SIGINT;
			break;
	}
	
	if (signal(sig, handler) == SIG_ERR) {
		return -1;
	}
	return 0;
}
