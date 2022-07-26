//
//  gltf.h
//  C-Ray
//
//  Created by Valtteri Koskivuori on 26/09/2021.
//  Copyright © 2021 Valtteri Koskivuori. All rights reserved.
//

#pragma once

// For now, we only support loading meshes from glTF files.
// Further down the line we could extend this with a glTF struct
// and allow loading more types of data from there.

struct mesh *parse_glTF_meshes(const char *filePath, size_t *meshCount);
