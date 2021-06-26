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
#include "../fileio.h"
#include "../logging.h"

struct mesh *loadMesh(char *filePath, size_t *meshCount) {
	switch (guessFileType(filePath)) {
		case obj:
			return parseWavefront(filePath, meshCount);
			break;
		default:
			logr(warning, "%s: Unknown file type, skipping.\n", filePath);
			return NULL;
	}
}
