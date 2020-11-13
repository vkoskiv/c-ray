//
//  testrunner.c
//  C-ray
//
//  Created by Valtteri on 23.6.2020.
//  Copyright © 2020 Valtteri Koskivuori. All rights reserved.
//

#include "../includes.h"
#include "logging.h"
#include "timer.h"

#include "../utils/assert.h"
#include "testrunner.h"

//Test assert
#define test_assert(x) if (!(x)) { pass = false; failed_expression = #x;}

typedef struct {
	char *testName;
	bool (*func)(void);
} test;

typedef struct {
	char *testName;
	time_t (*func)(void);
} perfTest;

// Grab tests
#include "../../tests/tests.h"
#include "../../tests/perf/tests.h"

#ifdef CRAY_TESTING

unsigned totalTests = testCount;
unsigned performanceTests = perfTestCount;

int runTests(void) {
	logr(info, "C-ray test framework v0.3\n");
	logr(info, "Running tests in a single process. Consider using run-tests.sh instead.\n");
	
	logr(info, "Running %u test%s.\n", totalTests, totalTests > 1 ? "s" : "");
	struct timeval t;
	startTimer(&t);
	for (unsigned t = 0; t < totalTests; ++t) {
		runTest(t);
	}
	logr(info, "Ran %u test%s in ", totalTests, totalTests > /* DISABLES CODE */ (1) ? "s" : "");
	printSmartTime(getMs(t));
	printf("\n");
	return 0;
}

int runPerfTests(void) {
	logr(info, "C-ray performance tests v0.1\n");
	logr(info, "Running performance tests in a single process. Consider using run-perf-tests.sh instead.\n");
	
	logr(info, "Running %u test%s.\n", performanceTests, performanceTests > 1 ? "s" : "");
	struct timeval t;
	startTimer(&t);
	for (unsigned t = 0; t < performanceTests; ++t) {
		runPerfTest(t);
	}
	logr(info, "Ran %u performance test%s in ", performanceTests, performanceTests > /* DISABLES CODE */ (1) ? "s" : "");
	printSmartTime(getMs(t));
	printf("\n");
	return 0;
}

int runTest(unsigned t) {
	t = t < totalTests ? t : totalTests - 1;
	logr(info,
		 "[%3u/%u] "
		 "%-32s ",
		 t + 1, totalTests,
		 tests[t].testName);
	
	struct timeval test;
	startTimer(&test);
	bool pass = tests[t].func();
	time_t usecs = getUs(test);
	
	printf(
		 "[%s%s%s] "
		 "(%6ld μs) "
		 "%s\n",
		 
		 pass ? KGRN : KRED, pass ? "PASS" : "FAIL", KNRM,
		 usecs,
		 pass ? "" : failed_expression ? failed_expression : "(no expression)"
	);
	failed_expression = NULL;
	return pass ? 0 : -1;
}

#define PERF_AVG_COUNT 100

int runPerfTest(unsigned t) {
	t = t < performanceTests ? t : performanceTests - 1;
	logr(info,
		 "[%3u/%u] "
		 "%-32s ",
		 t + 1, performanceTests,
		 tests[t].testName);
	
	
	time_t usecs = 0;
	
	for (size_t i = 0; i < PERF_AVG_COUNT; ++i) {
		usecs += perfTests[t].func();
	}
	
	usecs = usecs / PERF_AVG_COUNT;
	
	printf("(%6ld μs avg) \n", usecs);
	return 0;
}

int getTestCount(void) {
	return testCount;
}

int getPerfTestCount(void) {
	return perfTestCount;
}

#endif
