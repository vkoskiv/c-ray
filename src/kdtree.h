//
//  kdtree.h
//  C-Ray
//
//  Created by Valtteri Koskivuori on 28/04/2017.
//  Copyright © 2017 Valtteri Koskivuori. All rights reserved.
//

#pragma once

struct poly;
struct boundingBox;

struct kdTreeNode {
	//Need a bounding volume
	struct boundingBox *bbox;//Bounding box
	struct kdTreeNode *left; //Pointer to left child
	struct kdTreeNode *right;//Pointer to right child
	struct poly *polygons;   //Polygons within the bounding box
	int firstPolyIndex;      //For offset (C-Ray uses a global polygon array)
	int polyCount;           //Amount of polygons
	int depth;
};

//Builds a k-d tree and returns the root node
struct kdTreeNode *buildTree(struct poly *polys, int polyCount, int firstPolyIndex, int depth);
