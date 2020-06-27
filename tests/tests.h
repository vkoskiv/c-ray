//
//  tests.h
//  C-ray
//
//  Created by Valtteri on 24.6.2020.
//  Copyright © 2020 Valtteri Koskivuori. All rights reserved.
//

#pragma once

//Test assert
#define test_assert(x) if (!(x)) {pass = false; logr(warning, "Test failed: [%s]\n", #x);}

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
	{"transforms::determinant4x4", transform_determinant4x4},
	{"textbuffer::textview", textbuffer_textview},
	{"textbuffer::tokenizer", textbuffer_tokenizer}
};

#define testCount (sizeof(tests) / sizeof(test))
