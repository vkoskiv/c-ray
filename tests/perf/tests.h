//
//  tests.h
//  C-Ray
//
//  Created by Valtteri Koskivuori on 10/11/2020.
//  Copyright © 2020 Valtteri Koskivuori. All rights reserved.
//

static char *failed_expression;

// Testable modules
#include "perf_texture.h"
#include "perf_fileio.h"
#include "perf_base64.h"

static perfTest perfTests[] = {
	{"fileio::load", fileio_load},
	{"base64::bigfile_encode", base64_bigfile_encode},
	{"base64::bigfile_decode", base64_bigfile_decode},
};

#define perfTestCount (sizeof(perfTests) / sizeof(perfTest))

