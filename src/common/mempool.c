//
//  mempool.c
//  c-ray
//
//  Created by Valtteri Koskivuori on 05/12/2020.
//  Copyright © 2020-2022 Valtteri Koskivuori. All rights reserved.
//

#include <stdlib.h>
#include "mempool.h"
#include "logging.h"
#include "fileio.h"

struct block {
	size_t size;
	size_t capacity;
	struct block *prev;
	cray_max_align_t data[];
};

struct block *newBlock(struct block *prev, size_t initialSize) {
	struct block *newBlock = calloc(1, sizeof(*newBlock) + initialSize);
	newBlock->capacity = initialSize;
	newBlock->size = 0;
	newBlock->prev = prev;
	return newBlock;
}

void *allocBlock(struct block **head, size_t size) {
	if (size == 0) return NULL;
	
	// Round up for alignment
	size += sizeof(cray_max_align_t) - (size % sizeof(cray_max_align_t));
	
	if ((*head)->size + size > (*head)->capacity) {
		// Need to add a new block
		size_t nextSize = (*head)->capacity > size ? (*head)->capacity : size;
		char buf[64];
		logr(debug, "Appending a new block of size %s. Previous head occupancy: %s\n", human_file_size(nextSize, buf), human_file_size((*head)->size, buf));
		*head = newBlock(*head, nextSize);
	}
	
	void *ptr = (char *)(*head)->data + (*head)->size;
	(*head)->size += size;
	return ptr;
}

void destroyBlocks(struct block *head) {
	size_t numDestroyed = 0;
	size_t bytesfreed = 0;
	while (head) {
		struct block *prev = head->prev;
		bytesfreed += head->size;
		free(head);
		numDestroyed++;
		head = prev;
	}
	char *size = human_file_size(bytesfreed, NULL);
	logr(debug, "Destroyed %zu blocks, %s\n", numDestroyed, size);
	free(size);
}
