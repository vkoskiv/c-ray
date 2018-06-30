//
//  errorhandler.c
//  C-Ray
//
//  Created by Valtteri Koskivuori on 14/09/15.
//  Copyright (c) 2015-2018 Valtteri Koskivuori. All rights reserved.
//

#include "includes.h"
#include "errorhandler.h"

#include <stdarg.h>

//Terminal color codes
#define KNRM  "\x1B[0m"
#define KRED  "\x1B[31m"
#define KGRN  "\x1B[32m"
#define KYEL  "\x1B[33m"
#define KBLU  "\x1B[34m"
#define KMAG  "\x1B[35m"
#define KCYN  "\x1B[36m"
#define KWHT  "\x1B[37m"

void logr(enum logType type, const char *fmt, ...) {
	
	time_t curTime = time(NULL);
	struct tm time = *localtime(&curTime);
	
	switch (type) {
		case info:
			printf("[%sINFO%s]", KGRN, KNRM);
			break;
		case warning:
			printf("[%sWARN%s]", KYEL, KNRM);
			break;
		case error:
			printf("[%sERR %s]", KRED, KNRM);
			break;
		default:
			break;
	}
	
	//I want to pad seconds from 0-9 with a zero
	char secstring[3];
	if (time.tm_sec < 10) {
		sprintf(secstring, "0%i", time.tm_sec);
	} else {
		sprintf(secstring, "%i", time.tm_sec);
	}
	
	char minstring[3];
	if (time.tm_min < 10) {
		sprintf(minstring, "0%i", time.tm_min);
	} else {
		sprintf(minstring, "%i", time.tm_min);
	}
	
	printf("[%d-%d-%d %d:%s:%s]: ",
		   time.tm_year + 1900,
		   time.tm_mon + 1,
		   time.tm_mday,
		   time.tm_hour,
		   minstring,
		   secstring);
	
	//Handle formatted string
	char buf[512];
	va_list vl;
	va_start(vl, fmt);
	vsnprintf(buf, sizeof(buf), fmt, vl);
	va_end(vl);
	//And print it
	printf("%s", buf);
	
	if (type == error) {
		printf("Aborting due to previous error.");
		exit(-1);
	}
}

