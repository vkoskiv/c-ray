//
//  gltf.h
//  C-Ray
//
//  Created by Valtteri Koskivuori on 26/09/2021.
//  Copyright © 2021 Valtteri Koskivuori. All rights reserved.
//

#pragma once

/*#include <stdbool.h>

struct renderer;

// If only_assets == 0, then we treat this as a scene and apply everything.
// Otherwise it's just a format for asset transport.
int parse_glTF(struct renderer *r, char *input, bool only_assets);
*/
struct mesh *parseglTF(const char *filePath, size_t *meshCount);
