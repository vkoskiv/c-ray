//
//  sampler.c
//  c-ray
//
//  Created by Valtteri on 28.4.2020.
//  Copyright © 2020-2022 Valtteri Koskivuori. All rights reserved.
//

#include <stdint.h>
#include <stdlib.h>
#include "halton.h"
#include "hammersley.h"
#include "random.h"
#include "sampler.h"
#include "common.h"

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
			initRandom(&sampler->sampler.random, hash64(pixelIndex * maxPasses + pass));
			sampler->type = Random;
			break;
	}
}

float getDimension(struct sampler *sampler) {
	switch (sampler->type) {
		case Hammersley:
			return getHammersley(&sampler->sampler.hammersley);
		case Halton:
			return getHalton(&sampler->sampler.halton);
		case Random:
			return getRandom(&sampler->sampler.random);
	}
	return 0;
}

void destroySampler(struct sampler *sampler) {
	free(sampler);
}
