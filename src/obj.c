//
//  obj.c
//  C-Ray
//
//  Created by Valtteri Koskivuori on 27/04/2017.
//  Copyright © 2017 Valtteri Koskivuori. All rights reserved.
//

#include "includes.h"
#include "obj.h"

#include "poly.h"
#include "bbox.h"


void addTransform(struct crayOBJ *obj, struct matrixTransform transform) {
	if (obj->transformCount == 0) {
		obj->transforms = (struct matrixTransform*)calloc(1, sizeof(struct matrixTransform));
	} else {
		obj->transforms = (struct matrixTransform*)realloc(obj->transforms, (obj->transformCount + 1) * sizeof(struct matrixTransform));
	}
	obj->transforms[obj->transformCount] = transform;
	obj->transformCount++;
}

void transformMesh(struct crayOBJ *object) {
	for (int tf = 0; tf < object->transformCount; tf++) {
		//Perform transforms
		for (int p = object->firstPolyIndex; p < (object->firstPolyIndex + object->polyCount); p++) {
			for (int v = 0; v < polygonArray[p].vertexCount; v++) {
				transformVector(&vertexArray[polygonArray[p].vertexIndex[v]], &object->transforms[tf]);
			}
		}
		//Clear isTransformed flags
		for (int p = object->firstPolyIndex; p < object->firstPolyIndex + object->polyCount; p++) {
			for (int v = 0; v < polygonArray->vertexCount; v++) {
				vertexArray[polygonArray[p].vertexIndex[v]].isTransformed = false;
			}
		}
	}
}
