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
	return bbox;
}
