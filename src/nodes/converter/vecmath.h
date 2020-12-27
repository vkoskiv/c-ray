//
//  vecmath.h
//  C-Ray
//
//  Created by Valtteri Koskivuori on 28/12/2020.
//  Copyright © 2020 Valtteri Koskivuori. All rights reserved.
//

#pragma once

enum op {
	Add,
	Subtract,
	Multiply,
	Average,
	Dot,
	Cross,
	Normalize,
	Reflect,
	Length,
};

const struct vectorNode *newVecMath(const struct world *world, const struct vectorNode *A, const struct vectorNode *B, const enum op op);
