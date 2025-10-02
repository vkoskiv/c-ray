//
//  perf_fileio.h
//  c-ray
//
//  Created by Valtteri Koskivuori on 10/11/2020.
//  Copyright © 2020-2023 Valtteri Koskivuori. All rights reserved.
//

#include <v.h>

#include "../../src/common/fileio.h"
#include "../../src/common/cr_assert.h"

time_t fileio_load(void) {
	v_timer test = v_timer_start();
	
	file_data bigfile = file_load("input/venusscaled.obj");
	ASSERT(bigfile.items);
	
	time_t us = v_timer_get_us(test);
	file_free(&bigfile);
	return us;
}
