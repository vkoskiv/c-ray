//
// Created by vkoskiv on 12/12/21.
//

#pragma once

#include "../src/renderer/samplers/sampler.h"

bool test_halton(void) {
	struct sampler *sampler = newSampler();
	initSampler(sampler, Halton, 0, 1, 0);

	return true;
}

bool test_hammersley(void) {

	return true;
}

#define RUNS 128
bool test_pseudorandom(void) {
	// Just check we get the same sequence twice
	float first_run[RUNS];
	struct sampler *sampler = newSampler();
	initSampler(sampler, Random, 0, 0, 0);
	for (size_t i = 0; i < RUNS; ++i) {
		first_run[i] = getDimension(sampler);
	}
	destroySampler(sampler);
	float second_run[RUNS];
	sampler = newSampler();
	initSampler(sampler, Random, 0, 0, 0);
	for (size_t i = 0; i < RUNS; ++i) {
		second_run[i] = getDimension(sampler);
	}
	destroySampler(sampler);
	for (size_t i = 0; i < RUNS; ++i) {
		test_assert(first_run[i] == second_run[i]);
	}
	return true;
}