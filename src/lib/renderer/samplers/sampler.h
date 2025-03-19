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

struct sampler *sampler_new(void);

void sampler_init(struct sampler *sampler, enum samplerType type, int pass, int maxPasses, uint32_t pixelIndex);

float sampler_dimension(struct sampler *sampler);

void sampler_destroy(struct sampler *sampler);
