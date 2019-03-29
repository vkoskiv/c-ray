//
//  obj.c
//  C-ray
//
//  Created by Valtteri Koskivuori on 27/04/2017.
//  Copyright © 2015-2019 Valtteri Koskivuori. All rights reserved.
//

#include "../includes.h"
#include "obj.h"

#include "../acceleration/bbox.h"
#include "../acceleration/kdtree.h"

/*
 struct crayOBJ {
 //Vertices
 int vertexCount;
 int firstVectorIndex;
 
 //Normals
 int normalCount;
 int firstNormalIndex;
 
 //Texture coordinates
 int textureCount;
 int firstTextureIndex;
 
 //Faces
 int polyCount;
 int firstPolyIndex;
 
 //Transforms to perform before rendering
 int transformCount;
 struct matrixTransform *transforms;
 
 //Materials
 int materialCount;
 struct material *materials;
 
 //Root node of the kd-tree for this obj
 struct kdTreeNode *tree;
 
 char *objName;
 };
 */

//Parse given .obj and .mtl and return a crayOBJ
struct crayOBJ *parseOBJFile(char *fileName) {
	struct crayOBJ *obj = calloc(1, sizeof(struct crayOBJ));
	
	
	
	return obj;
}

void addTransform(struct crayOBJ *obj, struct matrixTransform transform) {
	if (obj->transformCount == 0) {
		obj->transforms = calloc(1, sizeof(struct matrixTransform));
	} else {
		obj->transforms = realloc(obj->transforms, (obj->transformCount + 1) * sizeof(struct matrixTransform));
	}
	obj->transforms[obj->transformCount] = transform;
	obj->transformCount++;
}

void transformMesh(struct crayOBJ *obj) {
	bool *tformed = (bool *)calloc(obj->polyCount*3, sizeof(bool *));
	for (int tf = 0; tf < obj->transformCount; tf++) {
		//Perform transforms
		for (int p = obj->firstPolyIndex; p < (obj->firstPolyIndex + obj->polyCount); p++) {
			for (int v = 0; v < polygonArray[p].vertexCount; v++) {
				transformVector(&vertexArray[polygonArray[p].vertexIndex[v]], &obj->transforms[tf], tformed[polygonArray[p].vertexIndex[v]]);
				tformed[polygonArray[p].vertexIndex[v]] = true;
			}
		}
		//Clear isTransformed flags
		for (int p = obj->firstPolyIndex; p < obj->firstPolyIndex + obj->polyCount; p++) {
			for (int v = 0; v < polygonArray->vertexCount; v++) {
				tformed[polygonArray[p].vertexIndex[v]] = false;
			}
		}
	}
	free(tformed);
}

void freeOBJ(struct crayOBJ *obj) {
	if (obj->objName) {
		free(obj->objName);
	}
	for (int i = 0; i < obj->materialCount; i++) {
		freeMaterial(&obj->materials[i]);
	}
	free(obj->materials);
	
	if (obj->tree) {
		freeTree(obj->tree);
	}
	free(obj->tree);
	
	free(obj->transforms);
}
