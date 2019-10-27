//
//  mesh.h
//  C-ray
//
//  Created by Valtteri Koskivuori on 27/04/2017.
//  Copyright © 2015-2019 Valtteri Koskivuori. All rights reserved.
//

#pragma once

struct boundingBox;

/*
 C-Ray stores all vectors and polygons in shared arrays, so these
 data structures just keep track of 'first-index offsets'
 These offsets are the element index of the first vector/polygon for this mesh
 The 'count' values are then used to keep track of where the vectors/polygons stop.
 
 Materials are stored within the mesh struct in *materials
 */

struct mesh {
	//Vertices
	int vertexCount;
	int firstVectorIndex;
	
	//Normals
	int normalCount;
	int firstNormalIndex;
	
	//Texture vec2inates
	int textureCount;
	int firstTextureIndex;
	
	//Faces
	int polyCount;
	int firstPolyIndex;
	
	//Transforms to perform before rendering
	int transformCount;
	struct transform *transforms;
	
	//Materials
	IMaterial mat;
	
	//Root node of the kd-tree for this mesh
	struct kdTreeNode *tree;
	
	char *name;
};

void addTransform(struct mesh *mesh, struct transform transform);
void transformMesh(struct mesh *mesh);

void freeMesh(struct mesh *mesh);
