//
//  obj.h
//  C-Ray
//
//  Created by Valtteri Koskivuori on 27/04/2017.
//  Copyright © 2017 Valtteri Koskivuori. All rights reserved.
//

#pragma once

struct boundingBox;

/*
 C-Ray stores all vectors and polygons in shared arrays, so these
 data structures just keep track of 'first-index offsets'
 These offsets are the element index of the first vector/polygon for this object
 The 'count' values are then used to keep track of where the vectors/polygons stop.
 */

struct crayOBJ {
	int vertexCount;
	int firstVectorIndex;
	
	int normalCount;
	int firstNormalIndex;
	
	int textureCount;
	int firstTextureIndex;
	
	int polyCount;
	int firstPolyIndex;
	
	struct matrixTransform *transforms;
	int transformCount;
	
	struct material *materials;
	int materialCount;
	
	//Root node of the kd-tree for this obj
	struct kdTreeNode *tree;
	
	char *objName;
};

void addTransform(struct crayOBJ *obj, struct matrixTransform transform);
void transformMesh(struct crayOBJ *object);
