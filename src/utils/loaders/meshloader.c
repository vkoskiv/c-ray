//
//  meshloader.c
//  C-ray
//
//  Created by Valtteri Koskivuori on 14.11.2019.
//  Copyright © 2019-2020 Valtteri Koskivuori. All rights reserved.
//

#include <stddef.h>

#include "meshloader.h"
#include "formats/wavefront/wavefront.h"

//TODO: Detect and support more formats than wavefront
struct mesh *loadMesh(char *filePath, size_t *meshCount) {
	return parseWavefront(filePath, meshCount);
}
