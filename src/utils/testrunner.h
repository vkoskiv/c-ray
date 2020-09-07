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
int runTests(void);
int runTest(unsigned);
int getTestCount(void);
#endif
