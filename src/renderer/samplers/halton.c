//
//  halton.c
//  C-ray
//
//  Created by Valtteri on 23.4.2020.
//  Copyright © 2020 Valtteri Koskivuori. All rights reserved.
//

#include "../../includes.h"
#include "halton.h"

#include "common.h"
#include "../../datatypes/vector.h"
#include "../../utils/assert.h"

static const unsigned int primes[] = {2, 3, 5, 7, 11, 13};
static const unsigned int primes_count = 6;

void initHalton(haltonSampler *s, int pass, uint32_t seed) {
	s->rndOffset = uintToUnitReal(seed);
	s->currPass = pass;
	s->currPrime = 0;
}

float getHalton(haltonSampler *s) {
	// Wrapping around trick by @lycium
	float v = wrapAdd(radicalInverse(s->currPass, primes[s->currPrime++ % primes_count]), s->rndOffset);
	ASSERT(v >= 0);
	ASSERT(v < 1);
	return v;
}
