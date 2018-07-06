//
//  logging.c
//  C-Ray
//
//  Created by Valtteri Koskivuori on 14/09/15.
//  Copyright (c) 2015-2018 Valtteri Koskivuori. All rights reserved.
//

#include "includes.h"
#include "logging.h"

#include <stdarg.h>
#include "renderer.h"

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
		printf("Aborting due to previous error.\n");
		exit(-1);
	}
}

void smartTime(unsigned long long milliseconds, char *buf) {
	time_t secs = milliseconds / 1000;
	time_t mins = secs / 60;
	time_t hours = (secs / 60) / 60;
	
	char secstring[25];
	unsigned long long remainderSeconds = secs - (mins * 60);
	if (remainderSeconds < 10) {
		sprintf(secstring, "0%llu", remainderSeconds);
	} else {
		sprintf(secstring, "%llu", remainderSeconds);
	}
	
	if (mins > 60) {
		sprintf(buf, "%lih %lim", hours, mins - (hours * 60));
	} else if (secs > 60) {
		sprintf(buf, "%lim %ss", mins, secstring);
	} else if (secs > 0) {
		sprintf(buf, "%.2fs", (float)milliseconds / 1000);
	} else {
		sprintf(buf, "%llums", milliseconds);
	}
}

/**
 Print running average duration of tiles rendered
 
 @param avgTime Current computed average time
 @param remainingTileCount Tiles remaining to render, to compute estimated remaining render time.
 */
void printStatistics(struct renderer *r, int thread, float kSamplesPerSecond) {
	int remainingTileCount = r->tileCount - r->finishedTileCount;
	unsigned long long remainingTimeMilliseconds = (remainingTileCount * r->avgTileTime) / r->threadCount;
	//First print avg tile time
	printf("%s", "\33[2K");
	float completion = ((float)r->finishedTileCount / r->tileCount) * 100;
	logr(info, "[%.0f%%]", completion);
	
	char avg[32];
	smartTime(r->avgTileTime, avg);
	printf(" avgt: %s", avg);
	char rem[32];
	smartTime(remainingTimeMilliseconds, rem);
	printf(", etf: %s, %.2fkS/s%s", rem, kSamplesPerSecond, "\r");
}

void computeStatistics(struct renderer *r, int thread, unsigned long long milliseconds, unsigned long long samples) {
	r->avgTileTime = r->avgTileTime * (r->timeSampleCount - 1);
	r->avgTileTime += milliseconds;
	r->avgTileTime /= r->timeSampleCount;
	
	float multiplier = (float)milliseconds / (float)1000.0f;
	float samplesPerSecond = (float)samples / multiplier;
	samplesPerSecond *= r->threadCount;
	r->avgSampleRate = r->avgSampleRate * (r->timeSampleCount - 1);
	r->avgSampleRate += samplesPerSecond;
	r->avgSampleRate /= (float)r->timeSampleCount;
	
	float printable = (float)r->avgSampleRate / 1000.0f;
	
	printStatistics(r, thread, printable);
	r->timeSampleCount++;
}

void printStats(struct renderer *r, unsigned long long ms, unsigned long long samples, int thread) {
#ifdef WINDOWS
	WaitForSingleObject(r->tileMutex, INFINITE);
#else
	pthread_mutex_lock(&r->tileMutex);
#endif
	computeStatistics(r, thread, ms, samples);
#ifdef WINDOWS
	ReleaseMutex(r->tileMutex);
#else
	pthread_mutex_unlock(&r->tileMutex);
#endif
}
