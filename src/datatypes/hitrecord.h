//
//  hitrecord.h
//  C-Ray
//
//  Created by Valtteri Koskivuori on 06/12/2020.
//  Copyright © 2020 Valtteri Koskivuori. All rights reserved.
//

#pragma once

#include "material.h"
#include "lightray.h"

struct hitRecord {
	struct lightRay incident;		//Light ray that encountered this intersection
	struct material material;		//Material of the intersected object
	struct vector hitPoint;			//Hit point vector in 3D space
	struct vector surfaceNormal;	//Surface normal at that point of intersection
	//TODO: Maybe a flag to indicate if we have UV instead of -1 sentinel value?
	// Do we even have cases without UV anymore?
	struct coord uv;				//UV barycentric coordinates for intersection point
	float distance;					//Distance to intersection point
	struct poly *polygon;			//ptr to polygon that was encountered
	int instIndex;					//Instance index, negative if no intersection
};
