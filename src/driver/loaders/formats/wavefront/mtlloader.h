//
//  mtlloader.h
//  C-ray
//
//  Created by Valtteri Koskivuori on 02/04/2019.
//  Copyright © 2019-2020 Valtteri Koskivuori. All rights reserved.
//

#pragma once

struct file_cache;
struct material_arr;

struct material_arr parse_mtllib(const char *filePath, struct file_cache *cache);
