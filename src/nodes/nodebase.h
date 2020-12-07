//
//  nodebase.h
//  C-Ray
//
//  Created by Valtteri Koskivuori on 07/12/2020.
//  Copyright © 2020 Valtteri Koskivuori. All rights reserved.
//

#pragma once

// Magic for comparing two nodes

struct nodeBase {
	bool (*compare)(const void *, const void *);
	uint32_t (*hash)(const void *);
};

bool compareNodes(const void *A, const void *B);
