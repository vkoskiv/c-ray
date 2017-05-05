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
	hitTypePolygon,
	hitTypeSphere,
	hitTypeNone
};

struct shadeInfo {
	struct vector hitPoint;
	struct vector normal;
	struct coord uv;
	struct material currentMaterial;
	enum currentType type;
	bool hasHit;
	int mtlIndex;
	int objIndex;
	double closestIntersection;
};

struct color rayTrace(struct lightRay *incidentRay, struct scene *worldScene);
