//
//  testrunner.c
//  C-ray
//
//  Created by Valtteri on 23.6.2020.
//  Copyright © 2020-2021 Valtteri Koskivuori. All rights reserved.
//

#include "../includes.h"
#include "logging.h"
#include "timer.h"

#include "../utils/assert.h"
#include "testrunner.h"

//Test assert
#define test_assert(x) if (!(x)) { failed_expression = #x; return false; }

// And an approximate one for math stuff
#define _roughly_equals(a, b, tolerance) \
	do { \
		float expect_close_lhs = a; \
		float expect_close_rhs = b; \
		float expect_close_diff = (float)(expect_close_lhs) - (float)(expect_close_rhs); \
		if (fabsf(expect_close_diff) > tolerance) { \
			failed_expression = "roughly_equals (" #a " !≈ " #b ")";\
			return false; \
		} \
	} while (false)

#define roughly_equals(a, b) _roughly_equals(a, b, 0.0000005)
#define very_roughly_equals(a, b) _roughly_equals(a, b, 0.01)

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

int firstTestIdx(char *suite);
int firstPerfTestIdx(char *suite);

// Grab tests
#include "../../tests/tests.h"
#include "../../tests/perf/tests.h"

unsigned totalTests = testCount;
unsigned performanceTests = perfTestCount;

int runTests(char *suite) {
	unsigned test_count = getTestCount(suite);
	logr(info, "C-ray test framework v0.3\n");
	logr(info, "Running tests in a single process. Consider using run-tests.sh instead.\n");
	
	logr(info, "Running %u test%s.\n", test_count, PLURAL(test_count));
	struct timeval t;
	timer_start(&t);
	for (unsigned t = 0; t < test_count; ++t) {
		runTest(t, suite);
	}
	logr(info, "Ran %u test%s in ", test_count, test_count > /* DISABLES CODE */ (1) ? "s" : "");
	printSmartTime(timer_get_ms(t));
	printf("\n");
	return 0;
}

#define PERF_AVG_COUNT 100

int runPerfTests(char *suite) {
	unsigned test_count = getPerfTestCount(suite);
	logr(info, "C-ray performance tests v0.1\n");
	logr(info, "Running performance tests in a single process. Consider using run-perf-tests.sh instead.\n");
	
	logr(info, "Running %u test%s.\n", test_count, PLURAL(test_count));
	logr(info, "Averaging runtime from %i runs for each test.\n", PERF_AVG_COUNT);
	struct timeval t;
	timer_start(&t);
	for (unsigned t = 0; t < test_count; ++t) {
		runPerfTest(t, suite);
	}
	logr(info, "Ran %u performance test%s in ", test_count, test_count > /* DISABLES CODE */ (1) ? "s" : "");
	printSmartTime(timer_get_ms(t));
	printf("\n");
	return 0;
}

int runTest(unsigned t, char *suite) {
	unsigned test_count = getTestCount(suite);
	unsigned first_idx = firstTestIdx(suite);
	t = t < test_count ? t : test_count - 1;
	logr(info,
		 "[%3u/%u] "
		 "%-32s ",
		 t + 1, test_count,
		 tests[first_idx + t].testName);
	
	struct timeval test;
	timer_start(&test);
	bool pass = tests[first_idx + t].func();
	time_t usecs = timer_get_us(test);
	
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

int runPerfTest(unsigned t, char *suite) {
	unsigned test_count = getPerfTestCount(suite);
	unsigned first_idx = firstPerfTestIdx(suite);
	t = t < test_count ? t : test_count - 1;
	logr(info,
		 "[%3u/%u] "
		 "%-32s ",
		 t + 1, test_count,
		 perfTests[first_idx + t].testName);
	
	
	time_t usecs = 0;
	
	for (size_t i = 0; i < PERF_AVG_COUNT; ++i) {
		usecs += perfTests[t].func();
	}
	
	usecs = usecs / PERF_AVG_COUNT;
	
	printf("(%6ld μs avg) \n", usecs);
	return 0;
}

int firstTestIdx(char *suite) {
	if (!suite) return 0;
	for (size_t i = 0; i < testCount; ++i) {
		if (stringStartsWith(suite, tests[i].testName)) return (int)i;
	}
	return 0;
}

int getTestCount(char *suite) {
	if (suite) {
		int test_count = 0;
		for (size_t i = 0; i < testCount; ++i) {
			if (stringStartsWith(suite, tests[i].testName)) test_count++;
		}
		return test_count;
	}
	return testCount;
}

int firstPerfTestIdx(char *suite) {
	if (!suite) return 0;
	for (size_t i = 0; i < perfTestCount; ++i) {
		if (stringStartsWith(suite, perfTests[i].testName)) return (int)i;
	}
	return 0;
}

int getPerfTestCount(char *suite) {
	if (suite) {
		int perf_test_count = 0;
		for (size_t i = 0; i < perfTestCount; ++i) {
			if (stringStartsWith(suite, perfTests[i].testName)) perf_test_count++;
		}
		return perf_test_count;
	}
	return perfTestCount;
}

#endif
