//
//  common.h
//  C-ray
//
//  Created by Valtteri on 28.4.2020.
//  Copyright © 2020 Valtteri Koskivuori. All rights reserved.
//

#pragma once

#include "../../../includes.h"

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

// By PBRT authors
static inline float radicalInverse(int pass, int base) {
	const float invBase = 1.0f / base;
	int reversedDigits = 0;
	float invBaseN = 1.0f;
	while (pass) {
		const int next = pass / base;
		const int digit = pass - base * next;
		reversedDigits = reversedDigits * base + digit;
		invBaseN *= invBase;
		pass = next;
	}
	return min(reversedDigits * invBaseN, 0.99999994f);
}

static inline float uintToUnitReal(uint32_t v) {
	// Trick from MTGP: generate an uniformly distributed single precision number in [1,2) and subtract 1
	union {
		uint32_t u;
		float f;
	} x;
	x.u = (v >> 9) | 0x3f800000u;
	return x.f - 1.0f;
}
