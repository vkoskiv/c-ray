//
//  nodebase.h
//  C-Ray
//
//  Created by Valtteri Koskivuori on 07/12/2020.
//  Copyright © 2020 Valtteri Koskivuori. All rights reserved.
//

#pragma once

#include "../utils/logging.h"

// Magic for comparing two nodes

struct nodeBase {
	bool (*compare)(const void *, const void *);
};

bool compareNodes(const void *A, const void *B);

#define HASH_CONS(hashtable, pool, hash, T, ...) \
	{ \
		T candidate = __VA_ARGS__; \
		uint32_t h = hash(&candidate); \
		T *existing = findInHashtable(hashtable, &candidate, h); \
		if (existing) {\
			logr(debug, "Reusing existing node!\n");\
			return (void *)existing; \
		} \
		T *final = allocBlock(pool, sizeof(T)); \
		memcpy(final, &candidate, sizeof(T)); \
		insertInHashtable(hashtable, &candidate, sizeof(T), h); \
		return (void *)final; \
	}
