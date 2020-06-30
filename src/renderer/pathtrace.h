//
//  pathtrace.h
//  C-ray
//
//  Created by Valtteri Koskivuori on 27/04/2017.
//  Copyright © 2015-2020 Valtteri Koskivuori. All rights reserved.
//

#pragma once

#include "../datatypes/vector.h"
#include "../datatypes/poly.h"
#include "../datatypes/lightRay.h"
#include "../datatypes/material.h"

struct world;

//FIXME: These datatypes should be hidden inside the implementation!

/**
 Ray intersection type enum
 */
enum currentType {
	hitTypePolygon,
	hitTypeSphere,
	hitTypeNone
};

/**
 Shading/intersection information, used to perform shading and rendering logic.
 @note uv, mtlIndex and polyIndex are only set if the ray hits a polygon (mesh)
 @todo Consider moving start, end materials to lightRay instead
 */
struct hitRecord {
	struct lightRay incident;		//Light ray that encountered this intersection
	struct material end;			//Material of the intersected object
	struct vector hitPoint;			//Hit point vector in 3D space
	struct vector surfaceNormal;	//Surface normal at that point of intersection
	struct coord uv;				//UV barycentric coordinates for intersection point
	enum currentType type;			//Type of object ray intersected with
	bool didIntersect;				//True if ray intersected
	float distance;					//Distance to intersection point
	struct poly *polygon;			//ptr to polygon that was encountered
	unsigned meshIndex : 16;
};


/// Recursive path tracer.
/// @param incidentRay View ray to be casted into the scene
/// @param scene Scene to cast the ray into
/// @param maxDepth Maximum depth of recursion
/// @param rng A random number generator. One per execution thread.
struct color pathTrace(const struct lightRay *incidentRay, const struct world *scene, int maxDepth, sampler *sampler);

struct color debugNormals(const struct lightRay *incidentRay, const struct world *scene, int maxDepth, sampler *sampler);
