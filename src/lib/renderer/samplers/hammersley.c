//
//  hammersley.c
//  c-ray
//
//  Created by Valtteri on 28.4.2020.
//  Copyright © 2020-2025 Valtteri Koskivuori. All rights reserved.
//

#include <stdint.h>
#include "hammersley.h"

#include "common.h"
#include <common/vector.h>
#include <common/cr_assert.h>

static const unsigned int primes_count = 6;

void initHammersley(hammersleySampler *s, int pass, int maxPasses, uint32_t seed) {
	s->rndOffset = uintToUnitReal(seed);
	s->currPass = pass;
	s->maxPasses = maxPasses;
	s->currPrime = 0;
}

// Wrong
float getHammersley(hammersleySampler *s) {
	// Wrapping around trick by Thomas Ludwig (@lycium)
	float u;
	if (s->currPass > 0) {
		u = radical_inverse(s->currPrime++ % primes_count, s->currPass);
	} else {
		u = s->currPass / s->maxPasses;
	}
	const float v = wrapAdd(u, s->rndOffset);
	ASSERT(v >= 0.0f);
	ASSERT(v <= 1.0f);
	return v;
}
