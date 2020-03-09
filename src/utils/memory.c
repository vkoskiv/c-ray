//
//  memory.c
//  C-ray
//
//  Created by Valtteri on 19.2.2020.
//  Copyright © 2020 Valtteri Koskivuori. All rights reserved.
//

#include "memory.h"
#include <stdlib.h>

#include "statistics.h"

void *cray_malloc(size_t size) {
	//increment(raw_bytes_allocated, size);
	//increment(calls_to_allocate, 1);
	return malloc(size);
}

void *cray_calloc(size_t count, size_t size) {
	//increment(raw_bytes_allocated, count * size);
	//increment(calls_to_allocate, 1);
	return calloc(count, size);
}

void cray_free(void *ptr) {
	//increment(raw_bytes_freed, ?);
	//increment(calls_to_free, 1);
	free(ptr);
}
