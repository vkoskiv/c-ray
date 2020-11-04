//
//  envmap.h
//  C-Ray
//
//  Created by Valtteri Koskivuori on 05/11/2020.
//  Copyright © 2020 Valtteri Koskivuori. All rights reserved.
//

#pragma once

struct texture;

struct envMap {
	struct texture *hdr;
	float offset; // In radians
};

struct envMap *newEnvMap(struct texture *);

void destroyEnvMap(struct envMap *map);
