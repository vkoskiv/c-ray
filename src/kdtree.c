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

//TODO: A func for bbox-ray intersection? May not be needed.

/*
 Nodes are built per-object
 Start at root node that contains all tris, and a bounding box for the obj
 At each level, split on a different axis in order X,Y,Z,X,Y,Z OR by longest axis
 For each level:
 1. Find the midpoint of all tris in the node (calculated in a bounding box already)
 2. Find the longest axis of the bounding box for that node
 3. For each tri in the node, check if for the current axis, it is less than or greater than the overall midpoint
 If less, push to left child
 if greater, push to right child
 */

struct kdTreeNode *getNewNode() {
	struct kdTreeNode *node = (struct kdTreeNode*)calloc(1, sizeof(struct kdTreeNode));
	node->bbox = NULL;
	node->left = NULL;
	node->right = NULL;
	node->polygons = NULL;
	node->polyCount = 0;
	return node;
}

void addPolyToArray(struct poly *array, struct poly *polygon) {
	//TODO:
}

struct kdTreeNode *buildTree(struct crayOBJ *obj, int depth) {
	struct kdTreeNode *node = (struct kdTreeNode*)calloc(1, sizeof(struct kdTreeNode));
	node->polygons = &polygonArray[obj->firstPolyIndex];
	node->polyCount = obj->polyCount;
	node->left = NULL;
	node->right = NULL;
	node->bbox = NULL;
	
	if (obj->polyCount == 0)
		return node;
	if (obj->polyCount == 1) {
		node->bbox = computeBoundingBox(&node->polygons[0], 1);
		node->left = getNewNode();
		node->right = getNewNode();
		return node;
	}
	
	node->bbox = computeBoundingBox(node->polygons, node->polyCount);
	
	struct vector midPoint = node->bbox->midPoint;
	
	struct poly leftPolys;
	struct poly rightPolys;
	
	//TODO: Enum axis
	int axis = getLongestAxis(node->bbox);
	
	for (int i = 0; i < node->polyCount; i++) {
		struct vector polyMidPoint = getMidPoint(&vertexArray[node->polygons[i].vertexIndex[0]],
									   &vertexArray[node->polygons[i].vertexIndex[1]],
									   &vertexArray[node->polygons[i].vertexIndex[2]]);
		switch (axis) {
			case 0:
				midPoint.x >= polyMidPoint.x ? addPolyToArray(&rightPolys, &node->polygons[i]) : addPolyToArray(&leftPolys, &node->polygons[i]);
				break;
				
			case 1:
				midPoint.y >= polyMidPoint.y ? addPolyToArray(&rightPolys, &node->polygons[i]) : addPolyToArray(&leftPolys, &node->polygons[i]);
				break;
				
			case 2:
				midPoint.z >= polyMidPoint.z ? addPolyToArray(&rightPolys, &node->polygons[i]) : addPolyToArray(&leftPolys, &node->polygons[i]);
				break;
		}
	}
	
	return node;
}
