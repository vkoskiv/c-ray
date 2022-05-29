//
//  vecmath.h
//  C-Ray
//
//  Created by Valtteri Koskivuori on 28/12/2020.
//  Copyright © 2020-2021 Valtteri Koskivuori. All rights reserved.
//

#pragma once

enum vecOp {
	VecAdd,
	VecSubtract,
	VecMultiply,
	VecAverage,
	VecDot,
	VecCross,
	VecNormalize,
	VecReflect,
	VecLength,
	VecAbs,
	VecMin,
	VecMax,
	VecFloor,
	VecCeil,
	VecSin,
	VecCos,
	VecTan,
	VecModulo,
};

const struct vectorNode *newVecMath(const struct world *world, const struct vectorNode *A, const struct vectorNode *B, const enum vecOp op);
