//
//  raytrace.h
//  C-Ray
//
//  Created by Valtteri Koskivuori on 27/04/2017.
//  Copyright © 2017 Valtteri Koskivuori. All rights reserved.
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
 NOTE: uv, mtlIndex and polyIndex are only set if the ray hits a polygon (OBJ mesh)
 */
struct intersection {
	struct lightRay ray;			//Light ray that encountered this intersection
	struct material start;			//Material of where that ray originates
	struct material end;			//Material of the intersected object
	struct vector hitPoint;			//Hit point vector in 3D space
	struct vector surfaceNormal;	//Surface normal at that point of intersection
	struct coord uv;				//UV barycentric coordinates for intersection point
	enum currentType type;			//Type of object ray intersected with
	bool didIntersect;				//True if ray intersected
	double distance;				//Distance to intersection point
	
	int mtlIndex;					//OBJ material index
	int polyIndex;					//OBJ polygon index
};

struct color rayTrace(struct lightRay *incidentRay, struct world *scene);

//New, experimental rayTracer
struct color newTrace(struct lightRay *incidentRay, struct world *scene);
