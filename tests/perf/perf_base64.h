//
//  perf_base64.h
//  c-ray
//
//  Created by Valtteri Koskivuori on 18/03/2021.
//  Copyright © 2021-2023 Valtteri Koskivuori. All rights reserved.
//

#include "../../src/utils/fileio.h"
#include "../../src/utils/assert.h"
#include "../../src/utils/base64.h"
#include "../../src/utils/timer.h"

time_t base64_bigfile_encode(void) {
	file_data bigfile = file_load("input/venusscaled.obj");
	ASSERT(bigfile.items);
	
	struct timeval test;
	timer_start(&test);
	
	char *encoded = b64encode(bigfile.items, bigfile.count);
	(void)encoded;
	
	time_t us = timer_get_us(test);
	file_free(&bigfile);
	free(encoded);
	return us;
}

time_t base64_bigfile_decode(void) {
	file_data bigfile = file_load("input/venusscaled.obj");
	ASSERT(bigfile.items);
	
	char *encoded = b64encode(bigfile.items, bigfile.count);
	size_t encodedLength = strlen(encoded);
	
	struct timeval test;
	timer_start(&test);
	
	char *decoded = b64decode(encoded, encodedLength, NULL);
	(void)decoded;
	
	time_t us = timer_get_us(test);
	file_free(&bigfile);
	free(encoded);
	free(decoded);
	return us;
}
