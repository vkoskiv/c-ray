//
//  logging.h
//  C-ray
//
//  Created by Valtteri Koskivuori on 14/09/2015.
//  Copyright © 2015-2020 Valtteri Koskivuori. All rights reserved.
//

#pragma once

struct renderer;

enum logType {
	error,
	info,
	warning,
	debug,
	plain
};

//Terminal color codes
//Xcode's awesome debugger doesn't have support for ANSI escapes,
//so I want to declutter the output when running there.
#ifndef XCODE_NO_COLOR
	#define KNRM  "\x1B[0m"
	#define KRED  "\x1B[31m"
	#define KGRN  "\x1B[32m"
	#define KYEL  "\x1B[33m"
	#define KBLU  "\x1B[34m"
	#define KMAG  "\x1B[35m"
	#define KCYN  "\x1B[36m"
	#define KWHT  "\x1B[37m"
#else
	#define KNRM  ""
	#define KRED  ""
	#define KGRN  ""
	#define KYEL  ""
	#define KBLU  ""
	#define KMAG  ""
	#define KCYN  ""
	#define KWHT  ""
#endif

#define PLURAL(x) (x) > 1 ? "s" : ""

/**
C-ray internal formatted logger.

@note Error will print stack trace and abort execution.
@param type Type of log message
@param fmt Log message formatted like printf
@param ...
*/
void logr(enum logType type, const char *fmt, ...)
#ifndef WINDOWS
			__attribute__ ((format (printf, 2, 3)))
#endif
;

void smartTime(unsigned long milliseconds, char *buf);

void printSmartTime(unsigned long long ms);
