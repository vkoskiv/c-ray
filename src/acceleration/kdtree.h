//
//  kdtree.h
//  C-ray
//
//  Created by Valtteri Koskivuori on 28/04/2017.
//  Copyright © 2015-2020 Valtteri Koskivuori. All rights reserved.
//

#pragma once

struct lightRay;
struct hitRecord;

struct kdTreeNode {
	struct boundingBox *bbox;//Bounding box
	struct kdTreeNode *left; //Pointer to left child
	struct kdTreeNode *right;//Pointer to right child
	int *polygons;   //indices to polygons within the bounding box
	int polyCount;   //Amount of polygons
};

/// Builds a KD-tree for a given array of polygons and returns a pointer to the root node
/// @param polygons Array of polygons to process
/// @param polyCount Amount of polygons given
/// @param depth Current depth for recursive calls
struct kdTreeNode *buildTree(int *polygons, const int polyCount);

/// Traverses a given KD-tree to find an intersection between a ray and a polygon in that tree. Hopefully really fast.
/// @param node Root node to start traversing from
/// @param ray Ray to check intersection against
/// @param isect Intersection information is saved to this struct
bool rayIntersectsWithNode(const struct kdTreeNode *node, const struct lightRay *ray, struct hitRecord *isect);

/// Count total nodes in a given tree
/// @param node root node of a tree to evaluate
int countNodes(const struct kdTreeNode *node);

/// Check the health of a given tree
/// @param node root node of a tree to evaluate
int checkTree(const struct kdTreeNode *node);

/// Free a given tree
/// @param node Root node of a tree to free
void freeTree(struct kdTreeNode *node);
