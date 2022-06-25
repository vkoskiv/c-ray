//
//  logging.c
//  C-ray
//
//  Created by Valtteri Koskivuori on 14/09/2015.
//  Copyright © 2015-2022 Valtteri Koskivuori. All rights reserved.
//

#include "../includes.h"
#include "logging.h"

#include <stdarg.h>
#include <time.h>
#include <stdio.h>
#include "args.h"
#include "platform/terminal.h"

char *color_escapes[] = {
	"\x1B[0m",
	"\x1B[31m",
	"\x1B[32m",
	"\x1B[33m",
	"\x1B[34m",
	"\x1B[35m",
	"\x1B[36m",
	"\x1B[37m",
};

char *colorEscape(int idx) {
	if (getenv("NO_COLOR"))
		return "";
	return color_escapes[idx];
}

static void printPrefix(enum logType type) {
	switch (type) {
		case info:
			printf("%sINFO%s ", KGRN, KNRM);
			break;
		case warning:
			printf("%sWARN%s ", KYEL, KNRM);
			break;
		case error:
			printf("%sERR %s ", KRED, KNRM);
			break;
		case debug:
			printf("%sDEBG%s ", KBLU, KNRM);
			break;
		default:
			break;
	}
}

static void printDate() {
	const time_t curTime = time(NULL);
	struct tm time = *localtime(&curTime);
	printf("[%d-%02d-%02d %02d:%02d:%02d] ",
		   time.tm_year + 1900,
		   time.tm_mon + 1,
		   time.tm_mday,
		   time.tm_hour,
		   time.tm_min,
		   time.tm_sec);
}

void logr(enum logType type, const char *fmt, ...) {
	if (!fmt) return;
	if (type == debug && !isSet("v")) return;
	
	if (type != plain) {
		printPrefix(type);
		printDate();
	}
	char buf[4096];
	int ret = 0;
	va_list vl;
	va_start(vl, fmt);
	ret += vsnprintf(buf, sizeof(buf), fmt, vl);
	va_end(vl);
	printf("%s", buf);
	if (ret > 4096) {
		// Overflowed, indicate that.
		printf("...\n");
	}
	if (type == error) {
		logr(info, "Aborting due to previous error.\n");
		restoreTerminal();
		exit(-1);
	}
}

void printSmartTime(unsigned long long ms) {
	char buf[64];
	smartTime(ms, buf);
	printf("%s", buf);
}

// Print to buf a logically formatted string representing time given in milliseconds.
// Example: "980ms", "2s", "1m 02s", "3h 12m", etc.
void smartTime(unsigned long milliseconds, char *buf) {
	time_t secs = milliseconds / 1000;
	time_t mins = secs / 60;
	time_t hours = (secs / 60) / 60;
	time_t days = hours / 24;
	
	if (days > 1) {
		sprintf(buf, "%lid %lih %lim", days, hours - (days * 24), mins - (hours * 60));
	} else if (mins > 60) {
		sprintf(buf, "%lih %lim", hours, mins - (hours * 60));
	} else if (secs > 60) {
		sprintf(buf, "%lim %02lds", mins, secs - (mins * 60));
	} else if (secs > 0) {
		sprintf(buf, "%.2fs", (float)milliseconds / 1000);
	} else {
		sprintf(buf, "%lums", milliseconds);
	}
}
