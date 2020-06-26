//
//  tests.h
//  C-ray
//
//  Created by Valtteri on 24.6.2020.
//  Copyright © 2020 Valtteri Koskivuori. All rights reserved.
//

#pragma once

// Testable modules
#include "test_textbuffer.h"
#include "test_transforms.h"
#include "test_vector.h"

typedef struct {
	char *testName;
	bool (*func)(void);
} test;

static test tests[] = {
	{"transforms::transpose", transform_transpose},
	{"transforms::multiply", transform_multiply},
	{"transforms::determinant", transform_determinant},
	{"transforms::determinant4x4", transform_determinant4x4}
};

#define testCount (sizeof(tests) / sizeof(test))
