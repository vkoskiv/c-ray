//
//  hitrecord.h
//  C-Ray
//
//  Created by Valtteri Koskivuori on 06/12/2020.
//  Copyright © 2020-2021 Valtteri Koskivuori. All rights reserved.
//

#pragma once

#include "material.h"
#include "lightray.h"

struct hitRecord {
	struct lightRay incident;		//Light ray that encountered this intersection
	struct vector hitPoint;			//Hit point vector in 3D space
	struct vector surfaceNormal;	//Surface normal at that point of intersection
	//FIXME: Do we even have cases without UV anymore?
	struct coord uv;				//UV barycentric coordinates for intersection point
	const struct bsdfNode *bsdf;	//Surface properties of the intersected object
	const struct color *emission;	//FIXME: Hack - Shouldn't have this here
	struct poly *polygon;			//ptr to polygon that was encountered
	float distance;					//Distance to intersection point
	int instIndex;					//Instance index, negative if no intersection
};
