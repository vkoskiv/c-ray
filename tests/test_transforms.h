//
//  test_transforms.h
//  C-ray
//
//  Created by Valtteri on 24.6.2020.
//  Copyright © 2020 Valtteri Koskivuori. All rights reserved.
//

#include "../src/datatypes/transforms.h"

// Grab private functions
//FIXME: This is a bit of a hack. Maybe find a better way?
float findDeterminant(float A[4][4], int n);
float findDeterminant4x4(float A[4][4]);
struct matrix4x4 fromParams(float t00, float t01, float t02, float t03,
							float t10, float t11, float t12, float t13,
							float t20, float t21, float t22, float t23,
							float t30, float t31, float t32, float t33);
struct matrix4x4 identityMatrix(void);
struct matrix4x4 multiply(struct matrix4x4 A, struct matrix4x4 B);
bool areEqual(struct matrix4x4 A, struct matrix4x4 B);

bool transform_multiply() {
	bool pass = true;
	struct matrix4x4 A = fromParams(
									5, 7, 9, 10,
									2, 3, 3, 8,
									8, 10, 2, 3,
									3, 3, 4, 8
									);
	struct matrix4x4 B = fromParams(
									3, 10, 12, 18,
									12, 1, 4, 9,
									9, 10, 12, 2,
									3, 12, 4, 10
									);
	struct matrix4x4 AB = multiply(A, B);
	struct matrix4x4 expectedResult = fromParams(
												 210, 267, 236, 271,
												 93, 149, 104, 149,
												 171, 146, 172, 268,
												 105, 169, 128, 169
												 );
	test_assert(areEqual(AB, expectedResult));
	return pass;
}

bool transform_transpose() {
	bool pass = true;
	
	// Identity matrices don't care about transpose
	test_assert(areEqual(identityMatrix(), transpose(identityMatrix())));
	
	struct matrix4x4 transposable = fromParams(
											0, 0, 0, 1,
											0, 0, 1, 0,
											0, 2, 0, 0,
											2, 0, 0, 0
											);
	
	struct matrix4x4 expected = fromParams(
										0, 0, 0, 2,
										0, 0, 2, 0,
										0, 1, 0, 0,
										1, 0, 0, 0
										);
	
	test_assert(areEqual(transpose(transposable), expected));
	
	return pass;
}

bool transform_determinant() {
	bool pass = true;
	struct matrix4x4 mtx = fromParams(1, 2, 0, 0,
									  1, 1, 3, 0,
									  0, 2, -2, 0,
									  0, 0, 3, 1);
	test_assert(findDeterminant(mtx.mtx, 4) == -4.0f);
	return pass;
}

bool transform_determinant4x4() {
	bool pass = true;
	struct matrix4x4 mtx = fromParams(1, 2, 0, 0,
									  1, 1, 3, 0,
									  0, 2, -2, 0,
									  0, 0, 3, 1);
	test_assert(findDeterminant4x4(mtx.mtx) == -4.0f);
	return pass;
}

// Rotations

// Translate

// Scale

// Inverse

// Determinant
