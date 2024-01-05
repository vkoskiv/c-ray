//
//  testrunner.h
//  c-ray
//
//  Created by Valtteri on 23.6.2020.
//  Copyright © 2020-2024 Valtteri Koskivuori. All rights reserved.
//

#pragma once

// c-ray integrated testing system.

int runTests(char *suite);
int runPerfTests(char *suite);
int runTest(unsigned test, char *suite);
int runPerfTest(unsigned test, char *suite);
int getTestCount(char *suite);
int getPerfTestCount(char *suite);
