//
//  nodebase.c
//  C-Ray
//
//  Created by Valtteri Koskivuori on 07/12/2020.
//  Copyright © 2020-2022 Valtteri Koskivuori. All rights reserved.
//

#include "nodebase.h"

bool compareNodes(const void *A, const void *B) {
	const struct nodeBase *node1 = (struct nodeBase *)A;
	const struct nodeBase *node2 = (struct nodeBase *)B;
	return node1->compare == node2->compare && node1->compare(node1, node2);
}
