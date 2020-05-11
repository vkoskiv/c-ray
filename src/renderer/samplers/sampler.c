//
//  sampler.c
//  C-ray
//
//  Created by Valtteri on 28.4.2020.
//  Copyright © 2020 Valtteri Koskivuori. All rights reserved.
//

#include <stdint.h>
#include <stdlib.h>
#include "halton.h"
#include "hammersley.h"
#include "random.h"
#include "sampler.h"
#include "common.h"
#include "../../utils/logging.h"

struct sampler {
	enum samplerType type;
	union {
		hammersleySampler hammersley;
		haltonSampler halton;
		randomSampler random;
	} sampler;
};

struct sampler *newSampler() {
	return calloc(1, sizeof(struct sampler));
}

void initSampler(sampler *sampler, enum samplerType type, int pass, int maxPasses, uint32_t pixelIndex) {
	switch (type) {
		case Halton:
			initHalton(&sampler->sampler.halton, pass, hash(pixelIndex));
			sampler->type = Halton;
			break;
		case Hammersley:
			initHammersley(&sampler->sampler.hammersley, pass, maxPasses, hash(pixelIndex));
			sampler->type = Hammersley;
			break;
		case Random:
			initRandom(&sampler->sampler.random, hash64(pixelIndex));
			sampler->type = Random;
			break;
	}
}

float getDimension(struct sampler *sampler) {
	float f;
	switch (sampler->type) {
		case Hammersley:
			f = getHammersley(&sampler->sampler.hammersley);
			break;
		case Halton:
			f = getHalton(&sampler->sampler.halton);
			break;
		case Random:
			f = getRandom(&sampler->sampler.random);
			break;
	}
	//logr(debug, "%.02f\n", f);
	return f;
}

void destroySampler(struct sampler *sampler) {
	if (sampler) free(sampler);
}
