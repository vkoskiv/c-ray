//
//  perf_texture.h
//  C-Ray
//
//  Created by Valtteri Koskivuori on 10/11/2020.
//  Copyright © 2020-2021 Valtteri Koskivuori. All rights reserved.
//

#include "../../src/datatypes/image/texture.h"
#include "../../src/utils/loaders/textureloader.h"
#include "../../src/utils/assert.h"

time_t texture_load(void) {
	struct timeval test;
	timer_start(&test);
	
	struct texture *new = load_texture("input/", NULL, NULL);
	
	ASSERT(new);
	
	time_t us = timer_get_us(test);
	destroyTexture(new);
	return us;
}
