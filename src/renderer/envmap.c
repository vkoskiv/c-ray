//
//  envmap.c
//  C-Ray
//
//  Created by Valtteri Koskivuori on 05/11/2020.
//  Copyright © 2020 Valtteri Koskivuori. All rights reserved.
//

#include "../includes.h"

#include "envmap.h"
#include "../datatypes/image/texture.h"

struct envMap *newEnvMap(struct texture *t) {
	struct envMap *new = calloc(1, sizeof(*new));
	new->hdr = t;
	new->offset = 0.0f;
	return new;
}

void destroyEnvMap(struct envMap *map) {
	if (map) {
		if (map->hdr) destroyTexture(map->hdr);
		free(map);
	}
}
