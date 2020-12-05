//
//  test_mempool.h
//  C-Ray
//
//  Created by Valtteri Koskivuori on 05/12/2020.
//  Copyright © 2020 Valtteri Koskivuori. All rights reserved.
//

#include "../src/utils/mempool.h"

size_t countBlocks(struct block *pool) {
	size_t blockAmount = 0;
	struct block *head = pool;
	while (head) {
		blockAmount++;
		head = head->prev;
	}
	return blockAmount;
}

bool mempool_bigalloc(void) {
	bool pass = true;
	
	struct block *pool = newBlock(NULL, 1024);
	size_t amount = 1000000;
	
	int *nums = allocBlock(&pool, amount * sizeof(*nums));
	
	test_assert(countBlocks(pool) == 2);
	test_assert(pool->prev->capacity == 1024);
	test_assert(pool->prev->size == 0);
	
	test_assert(pool->capacity == amount * sizeof(*nums));
	test_assert(pool->size == amount * sizeof(*nums));
	
	destroyBlocks(pool);
	
	return pass;
}

bool mempool_tiny_8(void) {
	bool pass = true;
	
	size_t blockSize = 8;
	struct block *pool = newBlock(NULL, blockSize);
	size_t amount = 1048576;
	
	for (size_t i = 0; i < amount; ++i) {
		allocBlock(&pool, sizeof(double));
	}
	
	test_assert(countBlocks(pool) == amount / (blockSize / sizeof(double)));
	
	destroyBlocks(pool);
	
	return pass;
}

bool mempool_tiny_16(void) {
	bool pass = true;
	
	size_t blockSize = 16;
	struct block *pool = newBlock(NULL, blockSize);
	size_t amount = 1048576;
	
	for (size_t i = 0; i < amount; ++i) {
		allocBlock(&pool, sizeof(double));
	}
	
	test_assert(countBlocks(pool) == amount / (blockSize / sizeof(double)));
	
	destroyBlocks(pool);
	
	return pass;
}

bool mempool_tiny_32(void) {
	bool pass = true;
	
	size_t blockSize = 32;
	struct block *pool = newBlock(NULL, blockSize);
	size_t amount = 1048576;
	
	for (size_t i = 0; i < amount; ++i) {
		allocBlock(&pool, sizeof(double));
	}
	
	test_assert(countBlocks(pool) == amount / (blockSize / sizeof(double)));
	
	destroyBlocks(pool);
	
	return pass;
}

bool mempool_tiny_64(void) {
	bool pass = true;
	
	size_t blockSize = 64;
	struct block *pool = newBlock(NULL, blockSize);
	size_t amount = 1048576;
	
	for (size_t i = 0; i < amount; ++i) {
		allocBlock(&pool, sizeof(double));
	}
	
	test_assert(countBlocks(pool) == amount / (blockSize / sizeof(double)));
	
	destroyBlocks(pool);
	
	return pass;
}

bool mempool_tiny_128(void) {
	bool pass = true;
	
	size_t blockSize = 128;
	struct block *pool = newBlock(NULL, blockSize);
	size_t amount = 1048576;
	
	for (size_t i = 0; i < amount; ++i) {
		allocBlock(&pool, sizeof(double));
	}
	
	test_assert(countBlocks(pool) == amount / (blockSize / sizeof(double)));
	
	destroyBlocks(pool);
	
	return pass;
}

bool mempool_tiny_256(void) {
	bool pass = true;
	
	size_t blockSize = 256;
	struct block *pool = newBlock(NULL, blockSize);
	size_t amount = 1048576;
	
	for (size_t i = 0; i < amount; ++i) {
		allocBlock(&pool, sizeof(double));
	}
	
	test_assert(countBlocks(pool) == amount / (blockSize / sizeof(double)));
	
	destroyBlocks(pool);
	
	return pass;
}

bool mempool_tiny_512(void) {
	bool pass = true;
	
	size_t blockSize = 512;
	struct block *pool = newBlock(NULL, blockSize);
	size_t amount = 1048576;
	
	for (size_t i = 0; i < amount; ++i) {
		allocBlock(&pool, sizeof(double));
	}
	
	test_assert(countBlocks(pool) == amount / (blockSize / sizeof(double)));
	
	destroyBlocks(pool);
	
	return pass;
}

bool mempool_tiny_1024(void) {
	bool pass = true;
	
	size_t blockSize = 1024;
	struct block *pool = newBlock(NULL, blockSize);
	size_t amount = 1048576;
	
	for (size_t i = 0; i < amount; ++i) {
		allocBlock(&pool, sizeof(double));
	}
	
	test_assert(countBlocks(pool) == amount / (blockSize / sizeof(double)));
	
	destroyBlocks(pool);
	
	return pass;
}

bool mempool_tiny_2048(void) {
	bool pass = true;
	
	size_t blockSize = 2048;
	struct block *pool = newBlock(NULL, blockSize);
	size_t amount = 1048576;
	
	for (size_t i = 0; i < amount; ++i) {
		allocBlock(&pool, sizeof(double));
	}
	
	test_assert(countBlocks(pool) == amount / (blockSize / sizeof(double)));
	
	destroyBlocks(pool);
	
	return pass;
}

bool mempool_tiny_4096(void) {
	bool pass = true;
	
	size_t blockSize = 4096;
	struct block *pool = newBlock(NULL, blockSize);
	size_t amount = 1048576;
	
	for (size_t i = 0; i < amount; ++i) {
		allocBlock(&pool, sizeof(double));
	}
	
	test_assert(countBlocks(pool) == amount / (blockSize / sizeof(double)));
	
	destroyBlocks(pool);
	
	return pass;
}
