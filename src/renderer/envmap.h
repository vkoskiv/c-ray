//
//  envmap.h
//  C-Ray
//
//  Created by Valtteri Koskivuori on 05/11/2020.
//  Copyright © 2020 Valtteri Koskivuori. All rights reserved.
//

#pragma once

struct texture;
struct lightRay;

struct envMap {
	struct texture *hdr;
	float offset; // In radians
};

struct envMap *newEnvMap(struct texture *);

struct color getEnvMap(const struct lightRay *incidentRay, const struct envMap *map);

void destroyEnvMap(struct envMap *map);
