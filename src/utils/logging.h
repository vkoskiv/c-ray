//
//  logging.h
//  C-ray
//
//  Created by Valtteri Koskivuori on 14/09/2015.
//  Copyright © 2015-2019 Valtteri Koskivuori. All rights reserved.
//

#pragma once

struct renderer;

enum logType {
	error,
	info,
	warning,
	debug
};

/**
C-ray internal formatted logger.

@note Error will print stack trace and abort execution.
@param type Type of log message
@param fmt Log message formatted like printf
@param ...
*/
void logr(enum logType type, const char *fmt, ...);

char *dateString(void);

void printStats(struct renderer *r, unsigned long long ms, unsigned long long samples, int thread);
