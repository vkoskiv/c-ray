//
//  sampler.h
//  c-ray
//
//  Created by Valtteri on 28.4.2020.
//  Copyright © 2020-2021 Valtteri Koskivuori. All rights reserved.
//

#pragma once

#include <stdint.h>

struct sampler;
typedef struct sampler sampler;

enum samplerType {
	Halton = 0,
	Hammersley,
	Random
};

struct sampler *newSampler(void);

void initSampler(struct sampler *sampler, enum samplerType type, int pass, int maxPasses, uint32_t pixelIndex);

float getDimension(struct sampler *sampler);

void destroySampler(struct sampler *sampler);
