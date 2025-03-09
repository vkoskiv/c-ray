//
//  signal.h
//  c-ray
//
//  Created by Valtteri on 7.4.2020.
//  Copyright © 2020 Valtteri Koskivuori. All rights reserved.
//

#pragma once

enum sigtype {
	sigint,
	sigabrt,
};

int registerHandler(enum sigtype, void (*handler)(int));

// By default, signals may be delivered to any running thread
// so block them in background threads.
// Thanks to jart for this TIL, I saw it in cosmo turfwar.c
void block_signals(void);
