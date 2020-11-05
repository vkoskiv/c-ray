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
#include "../datatypes/vector.h"
#include "../datatypes/lightRay.h"
#include "../datatypes/color.h"

struct envMap *newEnvMap(struct texture *t) {
	struct envMap *new = calloc(1, sizeof(*new));
	new->hdr = t;
	new->offset = 0.0f;
	return new;
}

struct color getEnvMap(const struct lightRay *incidentRay, const struct envMap *map) {
	//Unit direction vector
	struct vector ud = vecNormalize(incidentRay->direction);
	
	//To polar from cartesian
	float r = 1.0f; //Normalized above
	float phi = (atan2f(ud.z, ud.x) / 4.0f) + map->offset;
	float theta = acosf((-ud.y / r));
	
	float u = theta / PI;
	float v = (phi / (PI / 2.0f));
	
	u = wrapMinMax(u, 0.0f, 1.0f);
	v = wrapMinMax(v, 0.0f, 1.0f);
	
	float x = (v * map->hdr->width);
	float y = (u * map->hdr->height);
	
	return textureGetPixelFiltered(map->hdr, x, y);
}

void destroyEnvMap(struct envMap *map) {
	if (map) {
		if (map->hdr) destroyTexture(map->hdr);
		free(map);
	}
}
