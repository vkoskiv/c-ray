//
//  halton.h
//  C-ray
//
//  Created by Valtteri on 23.4.2020.
//  Copyright © 2020 Valtteri Koskivuori. All rights reserved.
//

#pragma once

struct haltonSampler {
	float rndOffset;
	unsigned currPrime;
	int currPass;
};

typedef struct haltonSampler haltonSampler;

void initHalton(haltonSampler *s, int pass, uint32_t seed);
float getHalton(haltonSampler *s);
