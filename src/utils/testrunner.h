//
//  testrunner.h
//  C-ray
//
//  Created by Valtteri on 23.6.2020.
//  Copyright © 2020 Valtteri Koskivuori. All rights reserved.
//

#pragma once

/*
 C-ray integrated testing system.
 */

#ifdef CRAY_TESTING
int runTests(char *suite);
int runPerfTests(char *suite);
int runTest(unsigned test, char *suite);
int runPerfTest(unsigned test, char *suite);
int getTestCount(char *suite);
int getPerfTestCount(char *suite);
#endif
