//
//  logging.c
//  C-ray
//
//  Created by Valtteri Koskivuori on 14/09/2015.
//  Copyright © 2015-2019 Valtteri Koskivuori. All rights reserved.
//

#include "../includes.h"
#include "logging.h"

#include <stdarg.h>
#include "../renderer/renderer.h"

//Terminal color codes
#define KNRM  "\x1B[0m"
#define KRED  "\x1B[31m"
#define KGRN  "\x1B[32m"
#define KYEL  "\x1B[33m"
#define KBLU  "\x1B[34m"
#define KMAG  "\x1B[35m"
#define KCYN  "\x1B[36m"
#define KWHT  "\x1B[37m"

void printPrefix(enum logType type) {
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
		case debug:
			printf("[%sDEBG%s]", KBLU, KNRM);
			break;
		default:
			break;
	}
}

char *dateString() {
	char *buf = malloc(20 * sizeof(char));
	time_t curTime = time(NULL);
	struct tm time = *localtime(&curTime);
	sprintf(buf, "%d-%02d-%02d_%02d_%02d_%02d",
			time.tm_year + 1900,
			time.tm_mon + 1,
			time.tm_mday,
			time.tm_hour,
			time.tm_min,
			time.tm_sec);
	
	buf[19] = '\0';
	return buf;
}

void printDate() {
	time_t curTime = time(NULL);
	struct tm time = *localtime(&curTime);
	printf("[%d-%02d-%02d %02d:%02d:%02d]: ",
		   time.tm_year + 1900,
		   time.tm_mon + 1,
		   time.tm_mday,
		   time.tm_hour,
		   time.tm_min,
		   time.tm_sec);
}

void logr(enum logType type, const char *fmt, ...) {
	if (!fmt) return;
	printPrefix(type);
	printDate();
	char buf[512];
	va_list vl;
	va_start(vl, fmt);
	vsnprintf(buf, sizeof(buf), fmt, vl);
	va_end(vl);
	printf("%s", buf);
	if (type == error) {
		logr(info, "Aborting due to previous error.\n");
		abort();
	}
}

// Print to buf a logically formatted string representing time given in milliseconds.
// Example: "980ms", "2s", "1m 02s", "3h 12m", etc.
void smartTime(unsigned long long milliseconds, char *buf) {
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
		sprintf(buf, "%llums", milliseconds);
	}
}

//FIXME: Clean up this mess here and move it elsewhere
/**
 Print running average duration of tiles rendered
 
 @param avgTime Current computed average time
 @param remainingTileCount Tiles remaining to render, to compute estimated remaining render time.
 */
void printStatistics(struct renderer *r, int thread, float kSamplesPerSecond) {
	int finishedTileCount = 0;
	for (int t = 0; t < r->prefs.threadCount; t++) {
		finishedTileCount += r->state.threadStates[t].finishedTileCount;
	}
	
	int remainingTileCount = r->state.tileCount - finishedTileCount;
	unsigned long long remainingTimeMilliseconds = (remainingTileCount * r->state.avgTileTime) / r->prefs.threadCount;
	//First print avg tile time
	printf("%s", "\33[2K");
	float completion = ((float)finishedTileCount / r->state.tileCount) * 100;
	logr(info, "[%.0f%%]", completion);
	
	char avg[32];
	smartTime(r->state.avgTileTime, avg);
	printf(" avgt: %s", avg);
	char rem[32];
	smartTime(remainingTimeMilliseconds, rem);
	printf(", etf: %s, %.2fkS/s%s", rem, kSamplesPerSecond, "\r");
}

void computeStatistics(struct renderer *r, int thread, unsigned long long milliseconds, unsigned long long samples) {
	r->state.avgTileTime = r->state.avgTileTime * (r->state.timeSampleCount - 1);
	r->state.avgTileTime += milliseconds;
	r->state.avgTileTime /= r->state.timeSampleCount;
	
	float multiplier = (float)milliseconds / (float)1000.0f;
	float samplesPerSecond = (float)samples / multiplier;
	samplesPerSecond *= r->prefs.threadCount;
	r->state.avgSampleRate = r->state.avgSampleRate * (r->state.timeSampleCount - 1);
	r->state.avgSampleRate += samplesPerSecond;
	r->state.avgSampleRate /= (float)r->state.timeSampleCount;
	
	float printable = (float)r->state.avgSampleRate / 1000.0f;
	
	printStatistics(r, thread, printable);
	r->state.timeSampleCount++;
}

void printStats(struct renderer *r, unsigned long long ms, unsigned long long samples, int thread) {
#ifdef WINDOWS
	WaitForSingleObject(r->state.statsMutex, INFINITE);
#else
	pthread_mutex_lock(&r->state.statsMutex);
#endif
	computeStatistics(r, thread, ms, samples);
#ifdef WINDOWS
	ReleaseMutex(r->state.statsMutex);
#else
	pthread_mutex_unlock(&r->state.statsMutex);
#endif
}
