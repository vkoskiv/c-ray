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

// Grab tests
#include "../../tests/tests.h"

#ifdef CRAY_TESTING
int runTests(void) {
	logr(info, "C-ray test framework v0.1\n");
	unsigned totalTests = testCount;
	logr(info, "Running %u test%s.\n", totalTests, totalTests > 1 ? "s" : "");
	struct timeval t;
	startTimer(&t);
	for (unsigned t = 0; t < testCount; ++t) {
		struct timeval test;
		startTimer(&test);
		bool pass = tests[t].func();
		time_t usecs = getUs(test);
		logr(pass ? info : warning,"[%i/%i] %-32s [%s%s%s] (%ldμs)\n", t + 1, totalTests, tests[t].testName, pass ? KGRN : KRED, pass ? "PASS" : "FAIL", KNRM, usecs);
	}
	logr(info, "Finished tests in ");
	printSmartTime(getMs(t));
	printf("\n");
	return 0;
}
#endif
