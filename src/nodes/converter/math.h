//
//  math.h
//  C-Ray
//
//  Created by Valtteri Koskivuori on 28/12/2020.
//  Copyright © 2020 Valtteri Koskivuori. All rights reserved.
//

#pragma once

enum mathOp {
	Add,
	Subtract,
	Multiply,
	Divide,
	Power,
	Log,
	SquareRoot,
	Absolute,
	Min,
	Max,
	Sine,
	Cosine,
	Tangent,
	ToRadians,
	ToDegrees,
};

const struct valueNode *newMath(const struct node_storage *s, const struct valueNode *A, const struct valueNode *B, const enum mathOp op);

