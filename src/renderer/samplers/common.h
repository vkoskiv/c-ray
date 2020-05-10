//
//  common.h
//  C-ray
//
//  Created by Valtteri on 28.4.2020.
//  Copyright © 2020 Valtteri Koskivuori. All rights reserved.
//

#pragma once

uint32_t hash(uint32_t x);
uint64_t hash64(uint64_t x);
float wrapAdd(float u, float v);
float radicalInverse(int pass, int base); // By PBRT authors
float uintToUnitReal(uint32_t v);
