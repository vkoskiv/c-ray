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
 
 Materials are stored within the crayOBJ struct in *materials
 */

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

void addTransform(struct crayOBJ *obj, struct matrixTransform transform);
void transformMesh(struct crayOBJ *object);

void freeOBJ(struct crayOBJ *obj);
