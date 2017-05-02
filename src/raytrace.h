//
//  raytrace.h
//  C-Ray
//
//  Created by Valtteri Koskivuori on 27/04/2017.
//  Copyright © 2017 Valtteri Koskivuori. All rights reserved.
//

#pragma once

struct scene;

enum currentType {
	polygon,
	sphere
};

struct shadeInfo {
	struct vector hitPoint;
	struct vector normal;
	struct material currentMaterial;
	enum currentType type;
	bool hasHit;
	int objIndex;
	double isectDistance;
};

struct color rayTrace(struct lightRay *incidentRay, struct scene *worldScene);
