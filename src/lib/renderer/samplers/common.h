//
//  common.h
//  c-ray
//
//  Created by Valtteri on 28.4.2020.
//  Copyright © 2020 Valtteri Koskivuori. All rights reserved.
//

#pragma once

#include "../../../includes.h"
#include <common/cr_assert.h>

// Hash function by Thomas Wang: https://burtleburtle.net/bob/hash/integer.html
static inline uint32_t hash(uint32_t x) {
	x  = (x ^ 12345391) * 2654435769;
	x ^= (x << 6) ^ (x >> 26);
	x *= 2654435769;
	x += (x << 5) ^ (x >> 12);
	return x;
}

static inline uint64_t hash64(uint64_t x) {
	x = (x ^ (x >> 30)) * UINT64_C(0xbf58476d1ce4e5b9);
	x = (x ^ (x >> 27)) * UINT64_C(0x94d049bb133111eb);
	x = x ^ (x >> 31);
	return x;
}

static inline float wrapAdd(float u, float v) {
	return (u + v < 1.0f) ? u + v : u + v - 1.0f;
}

#define RAD_INV_BASES \
	X(1, 3) \
	X(2, 5) \
	X(3, 7) \
	X(4, 11) \
	X(5, 13) \

// NOTE: +1 for case 0 reverse_bits_64() in radical_inverse().
static const int n_bases = 5 + 1;

static const float one_minus_epsilon = 0x1.fffffep-1;
// https://pbr-book.org/3ed-2018/Sampling_and_Reconstruction/The_Halton_Sampler#RadicalInverseSpecialized
#define X(idx, base) \
static inline float radical_inverse_b_##base(int a) { \
	const float inv_base = 1.0f / (float)base; \
	uint64_t reversed_digits = 0; \
	float inv_base_n = 1.0f; \
	while (a) { \
		const uint64_t next = a / base; \
		const uint64_t digit = a - next * base; \
		reversed_digits = reversed_digits * base + digit; \
		inv_base_n *= inv_base; \
		a = next; \
	} \
	return min(reversed_digits * inv_base_n, one_minus_epsilon); \
}

RAD_INV_BASES

#undef X

// https://pbr-book.org/3ed-2018/Sampling_and_Reconstruction/The_Halton_Sampler#ReverseBits32
static inline uint32_t reverse_bits_32(uint32_t n) {
	n = (n << 16) | (n >> 16);
	n = ((n & 0x00ff00ff) << 8) | ((n & 0xff00ff00) >> 8);
    n = ((n & 0x0f0f0f0f) << 4) | ((n & 0xf0f0f0f0) >> 4);
    n = ((n & 0x33333333) << 2) | ((n & 0xcccccccc) >> 2);
    n = ((n & 0x55555555) << 1) | ((n & 0xaaaaaaaa) >> 1);
    return n;
}

// https://pbr-book.org/3ed-2018/Sampling_and_Reconstruction/The_Halton_Sampler#ReverseBits64
static inline uint64_t reverse_bits_64(uint64_t n) {
	uint64_t n0 = reverse_bits_32((uint32_t)n);
	uint64_t n1 = reverse_bits_32((uint32_t)(n >> 32));
	return (n0 << 32) | n1;
}

#define X(idx, base) case idx: return radical_inverse_b_##base(a);
static inline float radical_inverse(int base_idx, uint64_t a) {
	ASSERT(a < n_bases);
	switch (base_idx) {
		case 0: return reverse_bits_64(a) * 0x1p-64;
		RAD_INV_BASES
	}
	ASSERT_NOT_REACHED();
	return 0.0f;
}
#undef X

static inline float uintToUnitReal(uint32_t v) {
	// Trick from MTGP: generate an uniformly distributed single precision number in [1,2) and subtract 1
	union {
		uint32_t u;
		float f;
	} x;
	x.u = (v >> 9) | 0x3f800000u;
	return x.f - 1.0f;
}
