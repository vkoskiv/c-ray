//
//  filecache.h
//  C-Ray
//
//  Created by Valtteri Koskivuori on 29/03/2021.
//  Copyright © 2021 Valtteri Koskivuori. All rights reserved.
//

#pragma once

#include <sys/types.h>

void cacheFile(const char *path, void *data, size_t length);

void *loadFromCache(const char *path, size_t *length);

char *encodeFileCache(void);

void decodeFileCache(const char *data);

void destroyFileCache(void);
