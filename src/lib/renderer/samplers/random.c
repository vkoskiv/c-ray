//
//  random.c
//  c-ray
//
//  Created by Valtteri on 28.4.2020.
//  Copyright © 2020-2025 Valtteri Koskivuori. All rights reserved.
//

#include "random.h"
#include <common/cr_assert.h>

void initRandom(randomSampler *s, uint64_t seed) {
	pcg32_srandom_r(&s->rng, seed, 0);
}

float getRandom(randomSampler *s) {
	const float v = (1.0f / (1ull << 32)) * pcg32_random_r(&s->rng);
	ASSERT(v >= 0);
	ASSERT(v <= 1);
	return v;
}
