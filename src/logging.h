//
//  logging.h
//  C-ray
//
//  Created by Valtteri Koskivuori on 14/09/2015.
//  Copyright © 2015-2018 Valtteri Koskivuori. All rights reserved.
//

#pragma once

struct renderer;

enum logType {
	error,
	info,
	warning
};

void initTerminal(void);

void logr(enum logType type, const char *fmt, ...);

void printStats(struct renderer *r, unsigned long long ms, unsigned long long samples, int thread);
