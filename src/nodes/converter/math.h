//
//  math.h
//  C-Ray
//
//  Created by Valtteri Koskivuori on 28/12/2020.
//  Copyright © 2020 Valtteri Koskivuori. All rights reserved.
//

#pragma once

// These are ripped off here:
// https://docs.blender.org/manual/en/latest/render/shader_nodes/converter/math.html
//TODO: Commented ones need to be implemented to reach parity with Cycles. Feel free to do so! :^)

enum mathOp {
	Add,
	Subtract,
	Multiply,
	Divide,
	//MultiplyAdd,
	Power,
	Log,
	SquareRoot,
	InvSquareRoot,
	Absolute,
	//Exponent,
	Min,
	Max,
	LessThan,
	GreaterThan,
	Sign,
	Compare,
	//SmoothMin,
	//SmoothMax,
	Round,
	Floor,
	Ceil,
	Truncate,
	Fraction,
	Modulo,
	//Wrap,
	//Snap,
	//PingPong,
	Sine,
	Cosine,
	Tangent,
	//ArcSine,
	//ArcCosine,
	//ArcTangent,
	//ArcTan2,
	//HyperbolicSine,
	//HyperbolicCosine,
	//HyperbolicTangent,
	ToRadians,
	ToDegrees,
};

const struct valueNode *newMath(const struct node_storage *s, const struct valueNode *A, const struct valueNode *B, const enum mathOp op);

