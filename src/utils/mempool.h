//
//  mempool.h
//  C-Ray
//
//  Created by Valtteri Koskivuori on 05/12/2020.
//  Copyright © 2020-2022 Valtteri Koskivuori. All rights reserved.
//

#pragma once

#include <stddef.h>
#include "memory.h"

struct block *newBlock(struct block *prev, size_t initialSize);

void *allocBlock(struct block **head, size_t size);

void destroyBlocks(struct block *head);
