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

// Grab tests
#include "../../tests/tests.h"

#ifdef CRAY_TESTING

unsigned totalTests = testCount;

int runTests(void) {
	logr(info, "C-ray test framework v0.2\n");
	logr(info, "Running tests in a single process. Consider using run-tests.sh instead.\n");
	
	logr(info, "Running %u test%s.\n", totalTests, totalTests > 1 ? "s" : "");
	struct timeval t;
	startTimer(&t);
	for (unsigned t = 0; t < testCount; ++t) {
		runTest(t);
	}
	logr(info, "Ran %lu test%s in ", testCount, testCount > /* DISABLES CODE */ (1) ? "s" : "");
	printSmartTime(getMs(t));
	printf("\n");
	return 0;
}

int runTest(unsigned t) {
	t = t < totalTests ? t : totalTests - 1;
	struct timeval test;
	startTimer(&test);
	bool pass = tests[t].func();
	time_t usecs = getUs(test);
	logr(pass ? info : warning,"[%3i/%i] %-32s [%s%s%s] (%ldμs)\n", t + 1, totalTests, tests[t].testName, pass ? KGRN : KRED, pass ? "PASS" : "FAIL", KNRM, usecs);
	return pass ? 0 : -1;
}

int getTestCount(void) {
	return testCount;
}

#endif
