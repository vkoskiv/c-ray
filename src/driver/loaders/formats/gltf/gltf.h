//
//  gltf.h
//  C-Ray
//
//  Created by Valtteri Koskivuori on 26/09/2021.
//  Copyright © 2021 Valtteri Koskivuori. All rights reserved.
//

#pragma once

struct mesh *parse_glTF_meshes(const char *filePath, size_t *meshCount);
