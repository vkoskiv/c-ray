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

void computeBoundingBox(struct crayOBJ *object) {
	struct vector minPoint = vertexArray[object->firstVectorIndex];
	struct vector maxPoint = vertexArray[object->firstVectorIndex];
	
	for (int i = object->firstVectorIndex + 1; i < (object->firstVectorIndex + object->vertexCount); i++) {
		minPoint = minVector(&minPoint, &vertexArray[i]);
		maxPoint = maxVector(&maxPoint, &vertexArray[i]);
	}
	struct vector center = vectorWithPos(0.5 * (minPoint.x + maxPoint.x), 0.5 * (minPoint.y + maxPoint.y), 0.5 * (minPoint.z + maxPoint.z));
	float maxDistance = 0.0;
	for (int i = object->firstVectorIndex + 1; i < (object->firstVectorIndex + object->vertexCount); i++) {
		struct vector fromCenter = subtractVectors(&vertexArray[i], &center);
		maxDistance = max(maxDistance, pow(vectorLength(&fromCenter), 2));
	}
	object->bbox->start = minPoint;
	object->bbox->  end = maxPoint;
}
