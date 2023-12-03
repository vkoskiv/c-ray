//
//  random.h
//  C-ray
//
//  Created by Valtteri on 28.4.2020.
//  Copyright © 2020 Valtteri Koskivuori. All rights reserved.
//

#pragma once

#include "../../vendored/pcg_basic.h"

struct randomSampler {
	pcg32_random_t rng;
};

typedef struct randomSampler randomSampler;

void initRandom(randomSampler *s, uint64_t seed);

float getRandom(randomSampler *s);
