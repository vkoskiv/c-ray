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
