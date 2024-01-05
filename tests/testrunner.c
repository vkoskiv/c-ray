//
//  testrunner.c
//  c-ray
//
//  Created by Valtteri on 23.6.2020.
//  Copyright © 2020-2024 Valtteri Koskivuori. All rights reserved.
//

#include "../src/includes.h"
#include "../src/common/logging.h"
#include "../src/common/timer.h"
#include "../src/driver/args.h"

#include "../src/common/assert.h"
#include "testrunner.h"

int firstTestIdx(char *suite);
int firstPerfTestIdx(char *suite);

// Grab tests
#include "tests.h"
#include "perf/tests.h"

unsigned totalTests = testCount;
unsigned performanceTests = perf_test_count;

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
		 tests[first_idx + t].test_name);
	
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
		 perf_tests[first_idx + t].test_name);
	
	
	time_t usecs = 0;
	
	for (size_t i = 0; i < PERF_AVG_COUNT; ++i) {
		usecs += perf_tests[t].func();
	}
	
	usecs = usecs / PERF_AVG_COUNT;
	
	printf("(%6ld μs avg) \n", usecs);
	return 0;
}

int firstTestIdx(char *suite) {
	if (!suite) return 0;
	for (size_t i = 0; i < testCount; ++i) {
		if (stringStartsWith(suite, tests[i].test_name)) return (int)i;
	}
	return 0;
}

int getTestCount(char *suite) {
	if (suite) {
		int suite_count = 0;
		for (size_t i = 0; i < testCount; ++i) {
			if (stringStartsWith(suite, tests[i].test_name)) suite_count++;
		}
		return suite_count;
	}
	return testCount;
}

int firstPerfTestIdx(char *suite) {
	if (!suite) return 0;
	for (size_t i = 0; i < perf_test_count; ++i) {
		if (stringStartsWith(suite, perf_tests[i].test_name)) return (int)i;
	}
	return 0;
}

int getPerfTestCount(char *suite) {
	if (suite) {
		int suite_count = 0;
		for (size_t i = 0; i < perf_test_count; ++i) {
			if (stringStartsWith(suite, perf_tests[i].test_name)) suite_count++;
		}
		return suite_count;
	}
	return perf_test_count;
}

int main(int argc, char *argv[]) {
	struct driver_args *args = args_parse(argc, argv);
	if (args_is_set(args, "runTests") || args_is_set(args, "runPerfTests")) {
		char *suite = args_string(args, "test_suite");
		int test_idx = args_int(args, "test_idx");
		switch (test_idx) {
			case -3:
				printf("%i", getPerfTestCount(suite));
				exit(0);
				break;
			case -2:
				printf("%i", getTestCount(suite));
				exit(0);
				break;
			case -1:
				exit(args_is_set(args, "runPerfTests") ? runPerfTests(suite) : runTests(suite));
				break;
			default:
				exit(args_is_set(args, "runPerfTests") ? runPerfTest(test_idx, suite) : runTest(test_idx, suite));
				break;
		}
	}
	return 0;
}
