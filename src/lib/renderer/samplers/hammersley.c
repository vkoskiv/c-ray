//
//  hammersley.c
//  C-ray
//
//  Created by Valtteri on 28.4.2020.
//  Copyright © 2020-2022 Valtteri Koskivuori. All rights reserved.
//

#include <stdint.h>
#include "hammersley.h"

#include "common.h"
#include "../../datatypes/vector.h"
#include "../../../common/assert.h"

static const unsigned int primes[] = {2, 3, 5, 7, 11, 13};
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
		u = radicalInverse(s->currPass, primes[s->currPrime++ % primes_count]);
	} else {
		u = s->currPass / s->maxPasses;
	}
	const float v = wrapAdd(u, s->rndOffset);
	ASSERT(v >= 0.0f);
	ASSERT(v <= 1.0f);
	return v;
}
