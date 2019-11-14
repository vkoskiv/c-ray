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

//Terminal color codes
#define KNRM  "\x1B[0m"
#define KRED  "\x1B[31m"
#define KGRN  "\x1B[32m"
#define KYEL  "\x1B[33m"
#define KBLU  "\x1B[34m"
#define KMAG  "\x1B[35m"
#define KCYN  "\x1B[36m"
#define KWHT  "\x1B[37m"

/**
C-ray internal formatted logger.

@note Error will print stack trace and abort execution.
@param type Type of log message
@param fmt Log message formatted like printf
@param ...
*/
void logr(enum logType type, const char *fmt, ...);

void smartTime(unsigned long long milliseconds, char *buf);
