//
//  perf_texture.h
//  C-Ray
//
//  Created by Valtteri Koskivuori on 10/11/2020.
//  Copyright © 2020 Valtteri Koskivuori. All rights reserved.
//

#include "../../src/datatypes/image/texture.h"
#include "../../src/utils/loaders/textureloader.h"
#include "../../src/utils/assert.h"

time_t texture_load(void) {
	struct timeval test;
	startTimer(&test);
	
	struct texture *new = load_texture("input/", NULL);
	
	ASSERT(new);
	
	time_t us = getUs(test);
	destroyTexture(new);
	return us;
}
