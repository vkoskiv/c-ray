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
#define test_assert(x) if (!(x)) { failed_expression = #x; return false; }

// And an approximate one for math stuff
#define roughly_equals(a, b) \
	do { \
		float expect_close_lhs = a; \
		float expect_close_rhs = b; \
		float expect_close_diff = (float)(expect_close_lhs) - (float)(expect_close_rhs); \
		if (fabsf(expect_close_diff) > 0.0000005) { \
			failed_expression = "roughly_equals (" #a " !≈ " #b ")";\
			return false; \
		} \
	} while (false)

#define vec_roughly_equals(veca, vecb) \
	do { \
		struct vector vec_a = veca; \
		struct vector vec_b = vecb; \
		roughly_equals(vec_a.x, vec_b.x); \
		roughly_equals(vec_a.y, vec_b.y); \
		roughly_equals(vec_a.z, vec_b.z); \
	} while (false)

typedef struct {
	char *testName;
	bool (*func)(void);
} test;

typedef struct {
	char *testName;
	time_t (*func)(void);
} perfTest;

#ifdef CRAY_TESTING

// Grab tests
#include "../../tests/tests.h"
#include "../../tests/perf/tests.h"

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

#define PERF_AVG_COUNT 100

int runPerfTests(void) {
	logr(info, "C-ray performance tests v0.1\n");
	logr(info, "Running performance tests in a single process. Consider using run-perf-tests.sh instead.\n");
	
	logr(info, "Running %u test%s.\n", performanceTests, performanceTests > 1 ? "s" : "");
	logr(info, "Averaging runtime from %i runs for each test.\n", PERF_AVG_COUNT);
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

int runPerfTest(unsigned t) {
	t = t < performanceTests ? t : performanceTests - 1;
	logr(info,
		 "[%3u/%u] "
		 "%-32s ",
		 t + 1, performanceTests,
		 perfTests[t].testName);
	
	
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
