//
//  kdtree.c
//  C-Ray
//
//  Created by Valtteri Koskivuori on 28/04/2017.
//  Copyright © 2017 Valtteri Koskivuori. All rights reserved.
//

#include "includes.h"
#include "kdtree.h"
#include "bbox.h"

//Tree funcs


struct kdTreeNode *buildTree(struct poly *polys, int depth) {
	//Todo
	struct kdTreeNode *node = (struct kdTreeNode*)calloc(1, sizeof(struct kdTreeNode));
	return node;
}
