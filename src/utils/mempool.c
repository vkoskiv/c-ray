//
//  mempool.c
//  C-Ray
//
//  Created by Valtteri Koskivuori on 05/12/2020.
//  Copyright © 2020 Valtteri Koskivuori. All rights reserved.
//

#include "../includes.h"
#include "mempool.h"

struct block *newBlock(struct block *prev, size_t initialSize) {
	struct block *newBlock = calloc(initialSize, sizeof(*newBlock));
	newBlock->capacity = initialSize;
	newBlock->size = 0;
	newBlock->prev = prev;
	return newBlock;
} 

void *allocBlock(struct block **head, size_t size) {
	if (size == 0) return NULL;
	
	if ((*head)->size + size > (*head)->capacity) {
		// Need to add a new block
		size_t nextSize = (*head)->capacity > size ? (*head)->capacity : size;
		*head = newBlock(*head, nextSize);
	}
	
	void *ptr = (char *)(*head)->data + (*head)->size;
	(*head)->size += size;
	return ptr;
}

void destroyBlocks(struct block *head) {
	while (head) {
		struct block *prev = head->prev;
		free(head);
		head = prev;
	}
}
