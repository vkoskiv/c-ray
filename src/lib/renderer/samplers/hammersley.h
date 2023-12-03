//
//  hammersley.h
//  C-ray
//
//  Created by Valtteri on 28.4.2020.
//  Copyright © 2020 Valtteri Koskivuori. All rights reserved.
//

#pragma once

#include <stddef.h>

struct hammersleySampler {
	float rndOffset;
	unsigned currPrime;
	int currPass;
	int maxPasses;
};

typedef struct hammersleySampler hammersleySampler;

void initHammersley(hammersleySampler *s, int pass, int maxPasses, uint32_t seed);
float getHammersley(hammersleySampler *s);
