//
//  tests.h
//  C-Ray
//
//  Created by Valtteri Koskivuori on 10/11/2020.
//  Copyright © 2020 Valtteri Koskivuori. All rights reserved.
//

static char *failed_expression;

//Test assert
#define test_assert(x) if (!(x)) { pass = false; failed_expression = #x;}

// Testable modules
#include "perf_texture.h"
#include "perf_fileio.h"

static perfTest perfTests[] = {
	{"fileio::load", fileio_load},
	
};

#define perfTestCount (sizeof(perfTests) / sizeof(perfTest))

