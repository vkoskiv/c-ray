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
struct matrix4x4 matrixFromParams(
	float t00, float t01, float t02, float t03,
	float t10, float t11, float t12, float t13,
	float t20, float t21, float t22, float t23,
	float t30, float t31, float t32, float t33);
struct matrix4x4 identityMatrix();

bool transform_multiply() {
	bool pass = true;
	struct matrix4x4 A = matrixFromParams(
		5, 7, 9, 10,
		2, 3, 3, 8,
		8, 10, 2, 3,
		3, 3, 4, 8);
	struct matrix4x4 B = matrixFromParams(
		3, 10, 12, 18,
		12, 1, 4, 9,
		9, 10, 12, 2,
		3, 12, 4, 10);
	struct matrix4x4 AB = multiplyMatrices(&A, &B);
	struct matrix4x4 expectedResult = matrixFromParams(
		210, 267, 236, 271,
		93, 149, 104, 149,
		171, 146, 172, 268,
		105, 169, 128, 169);
	test_assert(areMatricesEqual(&AB, &expectedResult));
	return pass;
}

bool transform_transpose() {
	bool pass = true;
	
	// Identity matrices don't care about transpose
	struct matrix4x4 id = identityMatrix();
	struct matrix4x4 tid = transposeMatrix(&id);
	test_assert(areMatricesEqual(&id, &tid));
	
	struct matrix4x4 transposable = matrixFromParams(
		0, 0, 0, 1,
		0, 0, 1, 0,
		0, 2, 0, 0,
		2, 0, 0, 0);
	
	struct matrix4x4 expected = matrixFromParams(
		0, 0, 0, 2,
		0, 0, 2, 0,
		0, 1, 0, 0,
		1, 0, 0, 0);
	struct matrix4x4 transposed = transposeMatrix(&transposable);
	
	test_assert(areMatricesEqual(&transposed, &expected));
	
	return pass;
}

bool transform_determinant() {
	bool pass = true;
	struct matrix4x4 mtx = matrixFromParams(
		1, 2, 0, 0,
		1, 1, 3, 0,
		0, 2, -2, 0,
		0, 0, 3, 1);
	test_assert(findDeterminant(mtx.mtx, 4) == -4.0f);
	return pass;
}

bool transform_determinant4x4() {
	bool pass = true;
	struct matrix4x4 mtx = matrixFromParams(
		1, 2, 0, 0,
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
