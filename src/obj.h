//
//  obj.h
//  C-Ray
//
//  Created by Valtteri Koskivuori on 27/04/2017.
//  Copyright © 2017 Valtteri Koskivuori. All rights reserved.
//

#pragma once

struct boundingBox;

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
	
	//struct boundingBox *bbox;
	struct kdTreeNode *tree;
	
	char *objName;
};

void addTransform(struct crayOBJ *obj, struct matrixTransform transform);
void transformMesh(struct crayOBJ *object);
