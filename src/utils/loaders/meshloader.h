//
//  meshloader.h
//  C-ray
//
//  Created by Valtteri Koskivuori on 14.11.2019.
//  Copyright © 2019-2022 Valtteri Koskivuori. All rights reserved.
//

#pragma once

struct file_cache;

struct mesh_arr load_meshes_from_file(const char *file_path, struct file_cache *cache);
