//
//  kdtree.c
//  C-ray
//
//  Created by Valtteri Koskivuori on 28/04/2017.
//  Copyright © 2015-2020 Valtteri Koskivuori. All rights reserved.
//

#include "../includes.h"
#include "kdtree.h"
#include "bbox.h"

#include "../renderer/pathtrace.h"
#include "../datatypes/vertexbuffer.h"
#include "../datatypes/poly.h"

//Tree funcs

/*
 Nodes are built per-object
 Start at root node that contains all tris, and a bounding box for the mesh
 At each level, split on a different axis in order X,Y,Z,X,Y,Z OR by longest axis
 For each level:
 1. Find the midpoint of all tris in the node (calculated in a bounding box already)
 2. Find the longest axis of the bounding box for that node
 3. For each tri in the node, check if for the current axis, it is less than or greater than the overall midpoint
 If less, push to left child
 if greater, push to right child
 */

struct Array {
	int *array;
	size_t used;
	size_t size;
};

void initArray(struct Array *a, size_t initialSize) {
	a->array = malloc(initialSize * sizeof(int));
	a->used = 0;
	a->size = initialSize;
}

void insertArray(struct Array *a, int element) {
	// a->used is the number of used entries, because a->array[a->used++] updates a->used only *after* the array has been accessed.
	// Therefore a->used can go up to a->size
	if (a->used == a->size) {
		a->size *= 2;
		a->array = realloc(a->array, a->size * sizeof(int));
	}
	a->array[a->used++] = element;
}

void freeArray(struct Array *a) {
	free(a->array);
	a->array = NULL;
	a->used = a->size = 0;
}

struct kdTreeNode *getNewNode() {
	struct kdTreeNode *node = calloc(1, sizeof(struct kdTreeNode));
	node->bbox = NULL;
	node->left = NULL;
	node->right = NULL;
	node->polygons = NULL;
	node->polyCount = 0;
	return node;
}

struct kdTreeNode *buildTree(int *polygons, const int polyCount) {
	struct kdTreeNode *node = getNewNode();
	node->polygons = polygons;
	node->polyCount = polyCount;
	
	if (polyCount == 0)
		return node;
	if (polyCount == 1) {
		node->bbox = computeBoundingBox(&node->polygons[0], 1);
		node->left = NULL;
		node->right = NULL;
		return node;
	}
	
	node->bbox = computeBoundingBox(node->polygons, node->polyCount);
	float currentSAHCost = node->polyCount * findSurfaceArea(node->bbox);
	
	struct vector midPoint = node->bbox->midPoint;
	
	struct Array leftPolys;
	initArray(&leftPolys, 5);
	struct Array rightPolys;
	initArray(&rightPolys, 5);
	
	enum bboxAxis axis = getLongestAxis(node->bbox);
	
	for (int i = 0; i < node->polyCount; i++) {
		struct vector polyMidPoint = getMidPoint(vertexArray[polygonArray[node->polygons[i]].vertexIndex[0]],
									   vertexArray[polygonArray[node->polygons[i]].vertexIndex[1]],
									   vertexArray[polygonArray[node->polygons[i]].vertexIndex[2]]);
		
		if (((axis == X) && (midPoint.x >= polyMidPoint.x)) ||
			((axis == Y) && (midPoint.y >= polyMidPoint.y)) ||
			((axis == Z) && (midPoint.z >= polyMidPoint.z))) {
			insertArray(&rightPolys, node->polygons[i]);
		} else {
			insertArray(&leftPolys, node->polygons[i]);
		}
	}
	
	bool rightFreed = false;
	bool leftFreed = false;
	if (leftPolys.used == 0 && rightPolys.used > 0) {
		freeArray(&leftPolys);
		leftPolys = rightPolys;
		leftFreed = true;
	}
	if (rightPolys.used == 0 && leftPolys.used > 0) {
		freeArray(&rightPolys);
		rightPolys = leftPolys;
		rightFreed = true;
	}
	
	struct boundingBox *leftBBox = computeBoundingBox(leftPolys.array, (int)leftPolys.used);
	struct boundingBox *rightBBox = computeBoundingBox(rightPolys.array, (int)rightPolys.used);
	
	float leftSAHCost = leftPolys.used * findSurfaceArea(leftBBox);
	float rightSAHCost = rightPolys.used * findSurfaceArea(rightBBox);
	
	free(leftBBox);
	free(rightBBox);
	
	if ((leftSAHCost + rightSAHCost) > currentSAHCost) {
		//Stop here
		node->left = NULL;
		node->right = NULL;
		if (leftPolys.used > 0 && !leftFreed) freeArray(&leftPolys);
		if (rightPolys.used > 0 && !rightFreed) freeArray(&rightPolys);
	} else {
		//Keep going
		node->left = buildTree(leftPolys.array, (int)leftPolys.used);
		node->right = buildTree(rightPolys.array, (int)rightPolys.used);
	}
	
	return node;
}

//Recurse through tree and count orphan nodes with no polygons
int checkTree(const struct kdTreeNode *node) {
	int orphans = 0;
	if (node) {
		if (node->polyCount == 0) {
			orphans += 1;
		}
		if (node->left) {
			orphans += checkTree(node->left);
		}
		if (node->right) {
			orphans += checkTree(node->right);
		}
	}
	return orphans;
}

int countNodes(const struct kdTreeNode *node) {
	int nodes = 0;
	if (node) {
		if (node->left) {
			nodes += countNodes(node->left);
		}
		if (node->right) {
			nodes += countNodes(node->right);
		}
		nodes += 1;
	}
	return nodes;
}

bool rayIntersectsWithNode(const struct kdTreeNode *node, const struct lightRay *ray, struct hitRecord *isect) {
	if (!node) return false;
	//A bit of a hack, but it does work...!
	float fakeIsect = 20000.0;
	if (rayIntersectWithAABB(node->bbox, ray, &fakeIsect)) {
		bool hasHit = false;
		
		if (node->left != NULL || node->right != NULL) {
			//Recurse down both sides
			bool hitLeft  = rayIntersectsWithNode(node->left, ray, isect);
			bool hitRight = rayIntersectsWithNode(node->right, ray, isect);
			
			return hitLeft || hitRight;
		} else {
			//This is a leaf, so check all polys
			for (int i = 0; i < node->polyCount; i++) {
				struct poly p = polygonArray[node->polygons[i]];
				if (rayIntersectsWithPolygon(ray, &p, &isect->distance, &isect->surfaceNormal, &isect->uv)) {
					hasHit = true;
					isect->type = hitTypePolygon;
					isect->polyIndex = p.polyIndex;
				}
			}
			if (hasHit) {
				isect->didIntersect = true;
				return true;
			} else {
				return false;
			}
		}
	}
	return false;
}

void destroyTree(struct kdTreeNode *node) {
	if (node) {
		destroyTree(node->left);
		destroyTree(node->right);
		if (node->bbox) free(node->bbox);
		if (node->polygons) free(node->polygons);
		free(node);
	}
}
