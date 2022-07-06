//
//  perf_fileio.h
//  C-Ray
//
//  Created by Valtteri Koskivuori on 10/11/2020.
//  Copyright © 2020 Valtteri Koskivuori. All rights reserved.
//

#include "../../src/utils/fileio.h"

time_t fileio_load(void) {
	struct timeval test;
	startTimer(&test);
	
	size_t bytes = 0;
	char *bigfile = loadFile("input/venusscaled.obj", &bytes, NULL);
	ASSERT(bigfile);
	
	time_t us = getUs(test);
	free(bigfile);
	return us;
}
