//
//  kdtree.c
//  C-Ray
//
//  Created by Valtteri Koskivuori on 28/04/2017.
//  Copyright © 2017 Valtteri Koskivuori. All rights reserved.
//

#include "includes.h"
#include "obj.h"
#include "kdtree.h"
#include "bbox.h"
#include "poly.h"

//Tree funcs

//TODO: Func that calculates a bounding box for a given set of points
//TODO: A func for bbox-ray intersection? May not be needed.

/*
 Nodes are built per-object
 Start at root node that contains all tris, and a bounding box for the obj
 At each level, split on a different axis in order X,Y,Z,X,Y,Z OR by longest axis
 For each level:
 1. Find the midpoint of all tris in the node
 2. Find the longest axis of the bounding box for that node
 3. For each tri in the node, check if for the current axis, it is less than or greater than the overall midpoint
 If less, push to left child
 if greater, push to right child
 */

struct kdTreeNode *buildTree(struct crayOBJ *obj, int depth) {
	//TODO
	struct kdTreeNode *node = (struct kdTreeNode*)calloc(1, sizeof(struct kdTreeNode));
	node->polygons = &polygonArray[obj->firstPolyIndex];
	node->polyCount = obj->polyCount;
	node->left = NULL;
	node->right = NULL;
	node->bbox = computeBoundingBox(node->polygons, node->polyCount);
	
	return node;
}
