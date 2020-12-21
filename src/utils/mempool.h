//
//  mempool.h
//  C-Ray
//
//  Created by Valtteri Koskivuori on 05/12/2020.
//  Copyright © 2020 Valtteri Koskivuori. All rights reserved.
//

#pragma once

#include "memory.h"

struct block {
	size_t size;
	size_t capacity;
	struct block *prev;
	cray_max_align_t data[];
};

struct block *newBlock(struct block *prev, size_t initialSize);

void *allocBlock(struct block **head, size_t size);

void destroyBlocks(struct block *head);
