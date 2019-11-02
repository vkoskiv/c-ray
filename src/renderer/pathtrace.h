//
//  pathtrace.h
//  C-ray
//
//  Created by Valtteri Koskivuori on 27/04/2017.
//  Copyright © 2015-2019 Valtteri Koskivuori. All rights reserved.
//

#pragma once

struct world;

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
struct intersection {
	struct lightRay ray;			//Light ray that encountered this intersection
	struct material *start;			//Material of where that ray originates
	struct material *end;			//Material of the intersected object
	vec3 hitPoint;					//Hit point vector in 3D space
	vec3 surfaceNormal;				//Surface normal at that point of intersection
	vec2 uv;						//UV barycentric coordinates for intersection point
	enum currentType type;			//Type of object ray intersected with
	bool didIntersect;				//True if ray intersected
	float distance;					//Distance to intersection point
	
	int mtlIndex;					//mesh material index
	int polyIndex;					//mesh polygon index
};


/// Recursive path tracer.
/// @param incidentRay View ray to be casted into the scene
/// @param scene Scene to cast the ray into
/// @param depth Current depth for recursive calls
/// @param maxDepth Maximum depth of recursion
/// @param rng A random number generator. One per execution thread.
/// @param hasHitObject set to true if an object was hit in this pass
color pathTrace(struct lightRay *incidentRay, struct world *scene, int maxDepth, pcg32_random_t *p_rng, bool *hasHitObject);
