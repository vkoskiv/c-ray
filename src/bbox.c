//
//  bbox.c
//  C-Ray
//
//  Created by Valtteri Koskivuori on 28/04/2017.
//  Copyright © 2017 Valtteri Koskivuori. All rights reserved.
//

#include "includes.h"
#include "bbox.h"

#include "obj.h"
#include "poly.h"

/**
 Get the longest axis of an axis-aligned bounding box
 
 @param bbox Bounding box to compute longest axis for
 @return Longest axis as an enum
 */
enum bboxAxis getLongestAxis(struct boundingBox *bbox) {
	int x = fabs(bbox->start.x - bbox->end.x);
	int y = fabs(bbox->start.y - bbox->end.y);
	int z = fabs(bbox->start.z - bbox->end.z);

	return x > y && x > z ? X : y > z ? Y : Z;
}


/**
 Compute the bounding box for a given array of polygons

 @param polys Polygons to compute bounding box for
 @param count Amount of polygons given
 @return Axis-aligned bounding box
 */
struct boundingBox *computeBoundingBox(struct poly *polys, int count) {
	struct boundingBox *bbox = (struct boundingBox*)calloc(1, sizeof(struct boundingBox));
	struct vector minPoint = vertexArray[polys[0].vertexIndex[0]];
	struct vector maxPoint = vertexArray[polys[0].vertexIndex[0]];
	
	for (int i = 0; i < count; i++) {
		for (int j = 0; j < 3; j++) {
			minPoint = minVector(&minPoint, &vertexArray[polys[i].vertexIndex[j]]);
			maxPoint = maxVector(&maxPoint, &vertexArray[polys[i].vertexIndex[j]]);
		}
	}
	struct vector center = vectorWithPos(0.5 * (minPoint.x + maxPoint.x), 0.5 * (minPoint.y + maxPoint.y), 0.5 * (minPoint.z + maxPoint.z));
	double maxDistance = 0.0;
	for (int i = 0; i < count; i++) {
		for (int j = 0; j < 3; j++) {
			struct vector fromCenter = subtractVectors(&vertexArray[polys[i].vertexIndex[j]], &center);
			maxDistance = max(maxDistance, pow(vectorLength(&fromCenter), 2));
		}
	}
	bbox->start = minPoint;
	bbox->end = maxPoint;
	bbox->midPoint = center;
	return bbox;
}


/**
 Check if a ray intersects with an axis-aligned bounding box

 @param box Given bounding box to check against
 @param ray Given light ray to intersect
 @param t Current max t value for the ray
 @return true if intersected, false otherwise
 */
bool rayIntersectWithAABB(struct boundingBox *box, struct lightRay *ray, double *t) {
	struct vector dirfrac;
	dirfrac.x = 1.0 / ray->direction.x;
	dirfrac.y = 1.0 / ray->direction.y;
	dirfrac.z = 1.0 / ray->direction.z;

	double t1 = (box->start.x - ray->start.x)*dirfrac.x;
	double t2 = (box->  end.x - ray->start.x)*dirfrac.x;
	double t3 = (box->start.y - ray->start.y)*dirfrac.y;
	double t4 = (box->  end.y - ray->start.y)*dirfrac.y;
	double t5 = (box->start.z - ray->start.z)*dirfrac.z;
	double t6 = (box->  end.z - ray->start.z)*dirfrac.z;
	
	double tmin = max(max(min(t1, t2), min(t3, t4)), min(t5, t6));
	double tmax = min(min(max(t1, t2), max(t3, t4)), max(t5, t6));
	
	// if tmax < 0, ray is intersecting AABB, but the whole AABB is behind us
	if (tmax < 0)
	{
		*t = tmax;
		return false;
	}
	
	// if tmin > tmax, ray doesn't intersect AABB
	if (tmin > tmax)
	{
		*t = tmax;
		return false;
	}
	
	*t = tmin;
	return true;
}
