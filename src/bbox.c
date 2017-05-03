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

//Return 0 if x, 1 if y, 2 if z
enum bboxAxis getLongestAxis(struct boundingBox *bbox) {
	int x = fabs(bbox->start.x - bbox->end.x);
	int y = fabs(bbox->start.y - bbox->end.y);
	int z = fabs(bbox->start.z - bbox->end.z);

	return x > y && x > z ? X : y > z ? Y : Z;
}

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
	float maxDistance = 0.0;
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

//Check if a ray intersects with an axis-aligned bounding box
bool rayIntersectWithAABB(struct boundingBox *box, struct lightRay *ray, double *t) {
	struct vector dirfrac;
	dirfrac.x = 1.0f / ray->direction.x;
	dirfrac.y = 1.0f / ray->direction.y;
	dirfrac.z = 1.0f / ray->direction.z;

	float t1 = (box->start.x - ray->start.x)*dirfrac.x;
	float t2 = (box->  end.x - ray->start.x)*dirfrac.x;
	float t3 = (box->start.y - ray->start.y)*dirfrac.y;
	float t4 = (box->  end.y - ray->start.y)*dirfrac.y;
	float t5 = (box->start.z - ray->start.z)*dirfrac.z;
	float t6 = (box->  end.z - ray->start.z)*dirfrac.z;
	
	double tmin = max(max(min(t1, t2), min(t3, t4)), min(t5, t6));
	double tmax = min(min(max(t1, t2), max(t3, t4)), max(t5, t6));
	
	// if tmax < 0, ray is intersecting AABB, but the whole AABB is behind us
	if (tmax < 0)
	{
		t = &tmax;
		return false;
	}
	
	// if tmin > tmax, ray doesn't intersect AABB
	if (tmin > tmax)
	{
		t = &tmax;
		return false;
	}
	
	t = &tmin;
	return true;
}
