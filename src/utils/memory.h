//
//  memory.h
//  C-ray
//
//  Created by Valtteri on 19.2.2020.
//  Copyright © 2020 Valtteri Koskivuori. All rights reserved.
//

#pragma once

#include <stdlib.h>

void *cray_malloc(size_t size);

void *cray_calloc(size_t count, size_t size);

void cray_free(void *ptr);
