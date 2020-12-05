//
//  test_mempool.h
//  C-Ray
//
//  Created by Valtteri Koskivuori on 05/12/2020.
//  Copyright © 2020 Valtteri Koskivuori. All rights reserved.
//

#include "../src/utils/mempool.h"

bool mempool_bigalloc(void) {
	bool pass = true;
	
	struct block *pool = newBlock(NULL, 1024);
	size_t amount = 1000000;
	
	int *nums = allocBlock(&pool, amount * sizeof(*nums));
	
	destroyBlocks(pool);
	
	return pass;
}

bool mempool_tiny(void) {
	bool pass = true;
	
	struct block *pool = newBlock(NULL, 1024);
	size_t amount = 1000000;
	
	for (size_t i = 0; i < amount; ++i) {
		allocBlock(&pool, sizeof(int));
	}
	
	size_t blockAmount = 0;
	struct block *head = pool;
	while (head) {
		blockAmount++;
		head = head->prev;
	}
	
	test_assert(blockAmount == 3907);
	test_assert(pool->size == 256);
	
	destroyBlocks(pool);
	
	return pass;
}
