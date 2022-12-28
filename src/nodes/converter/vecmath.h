//
//  vecmath.h
//  C-Ray
//
//  Created by Valtteri Koskivuori on 28/12/2020.
//  Copyright © 2020-2021 Valtteri Koskivuori. All rights reserved.
//

#pragma once

// These are ripped off here:
// https://docs.blender.org/manual/en/latest/render/shader_nodes/converter/vector_math.html
//TODO: Commented ones need to be implemented to reach parity with Cycles. Feel free to do so! :^)

enum vecOp {
	VecAdd,
	VecSubtract,
	VecMultiply,
	VecDivide,
	//VecMultiplyAdd,
	VecCross,
	//VecProject,
	VecReflect,
	VecRefract,
	//VecFaceforward,
	VecDot,
	VecDistance,
	VecLength,
	VecScale,
	VecNormalize,
	VecWrap,
	//VecSnap,
	VecFloor,
	VecCeil,
	VecModulo,
	//VecFraction,
	VecAbs,
	VecMin,
	VecMax,
	VecSin,
	VecCos,
	VecTan,
};

const struct vectorNode *newVecMath(const struct node_storage *s, const struct vectorNode *A, const struct vectorNode *B, const struct vectorNode *C, const struct valueNode *f, const enum vecOp op);
