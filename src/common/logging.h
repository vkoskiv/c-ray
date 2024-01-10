//
//  logging.h
//  c-ray
//
//  Created by Valtteri Koskivuori on 14/09/2015.
//  Copyright © 2015-2024 Valtteri Koskivuori. All rights reserved.
//

#pragma once

#include "../../include/c-ray/c-ray.h"

struct renderer;

enum logType {
	error,
	info,
	warning,
	debug,
	plain,
	spam, // Mostly node dumps, easily overwhelms a terminal emulator in a complex scene :]
};

char *colorEscape(int idx);

//Terminal color codes
//Xcode's awesome debugger doesn't have support for ANSI escapes,
//so I want to declutter the output when running there.
#ifndef NO_COLOR
	#define KNRM colorEscape(0)
	#define KRED colorEscape(1)
	#define KGRN colorEscape(2)
	#define KYEL colorEscape(3)
	#define KBLU colorEscape(4)
	#define KMAG colorEscape(5)
	#define KCYN colorEscape(6)
	#define KWHT colorEscape(7)
#else
	#define KNRM ""
	#define KRED ""
	#define KGRN ""
	#define KYEL ""
	#define KBLU ""
	#define KMAG ""
	#define KCYN ""
	#define KWHT ""
#endif

#define PLURAL(x) (x) > 1 ? "s" : (x) == 0 ? "s" : ""

void log_level_set(enum cr_log_level);
enum cr_log_level log_level_get(void);

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

char *ms_to_readable(unsigned long milliseconds, char *buf);

void printSmartTime(unsigned long long ms);
