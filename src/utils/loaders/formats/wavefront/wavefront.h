//
//  wavefront.h
//  C-ray
//
//  Created by Valtteri Koskivuori on 02/04/2019.
//  Copyright © 2019-2021 Valtteri Koskivuori. All rights reserved.
//

#pragma once

struct file_cache;

struct mesh *parseWavefront(const char *filePath, size_t *meshCount, struct file_cache *cache);
