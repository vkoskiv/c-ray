//
//  perf_fileio.h
//  c-ray
//
//  Created by Valtteri Koskivuori on 10/11/2020.
//  Copyright © 2020-2023 Valtteri Koskivuori. All rights reserved.
//

#include "../../src/utils/fileio.h"
#include "../../src/utils/timer.h"
#include "../../src/utils/assert.h"

time_t fileio_load(void) {
	struct timeval test;
	timer_start(&test);
	
	file_data bigfile = file_load("input/venusscaled.obj");
	ASSERT(bigfile.items);
	
	time_t us = timer_get_us(test);
	file_free(&bigfile);
	return us;
}
