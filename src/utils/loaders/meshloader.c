//
//  meshloader.c
//  C-ray
//
//  Created by Valtteri Koskivuori on 14.11.2019.
//  Copyright © 2019-2022 Valtteri Koskivuori. All rights reserved.
//

#include <stddef.h>

#include "meshloader.h"
#include "formats/wavefront/wavefront.h"
#include "../fileio.h"
#include "../logging.h"

struct mesh *load_meshes_from_file(const char *filePath, size_t *meshCount, struct file_cache *cache) {
	switch (guessFileType(filePath)) {
		case obj:
			return parseWavefront(filePath, meshCount, cache);
		default:
			logr(warning, "%s: Unknown file type, skipping.\n", filePath);
			if (meshCount) *meshCount = 0;
			return NULL;
	}
}