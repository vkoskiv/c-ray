//
//  test_transforms.h
//  C-ray
//
//  Created by Valtteri on 24.6.2020.
//  Copyright © 2020 Valtteri Koskivuori. All rights reserved.
//

#include "../src/datatypes/transforms.h"
#include "../src/datatypes/vector.h"

// Grab private functions
float findDeterminant(float A[4][4], int n);
float findDeterminant4x4(float A[4][4]);
struct matrix4x4 matrixFromParams(
	float t00, float t01, float t02, float t03,
	float t10, float t11, float t12, float t13,
	float t20, float t21, float t22, float t23,
	float t30, float t31, float t32, float t33);

struct matrix4x4 identityMatrix(void);

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

bool transform_rotate_X() {
	bool pass = true;
	
	struct transform rotX = newTransformRotateX(toRadians(45.0f));
	struct vector vec = (struct vector){0.5f, 0.0f, 0.0f};
	//vec = vecNormalize(vec);
	transformPoint(&vec, &rotX.A);
	test_assert(vecEquals(vec, (struct vector){0.0f, 1.0f, 0.0f}));
	
	return pass;
}

bool transform_rotate_Y() {
	bool pass = true;
	pass = false;
	return pass;
}

bool transform_rotate_Z() {
	bool pass = true;
	pass = false;
	return pass;
}

// Translate

bool transform_translate_X() {
	bool pass = true;
	struct vector vec = (struct vector){-10.0f, 0.0f, 0.0f};
	
	struct transform tr = newTransformTranslate(1.0f, 0.0f, 0.0f);
	transformPoint(&vec, &tr.A);
	test_assert(vecEquals(vec, (struct vector){-9.0f, 0.0f, 0.0f}));
	
	return pass;
}

bool transform_translate_Y() {
	bool pass = true;
	struct vector vec = (struct vector){0.0f, -10.0f, 0.0f};
	
	struct transform tr = newTransformTranslate(0.0f, 1.0f, 0.0f);
	transformPoint(&vec, &tr.A);
	test_assert(vecEquals(vec, (struct vector){0.0f, -9.0f, 0.0f}));
	
	return pass;
}

bool transform_translate_Z() {
	bool pass = true;
	struct vector vec = (struct vector){0.0f, 0.0f, -10.0f};
	
	struct transform tr = newTransformTranslate(0.0f, 0.0f, 1.0f);
	transformPoint(&vec, &tr.A);
	test_assert(vecEquals(vec, (struct vector){0.0f, 0.0f, -9.0f}));
	
	return pass;
}

bool transform_translate_all() {
	bool pass = true;
	struct vector vec = (struct vector){0.0f, 0.0f, 0.0f};
	
	struct transform tr = newTransformTranslate(-1.0f, -10.0f, -100.0f);
	transformPoint(&vec, &tr.A);
	test_assert(vecEquals(vec, (struct vector){-1.0f, -10.0f, -100.0f}));
	
	return pass;
}


// Scale
bool transform_scale_X() {
	bool pass = true;
	struct vector vec = (struct vector){-10.0f, 0.0f, 0.0f};
	
	struct transform tr = newTransformScale(3.0f, 1.0f, 1.0f);
	transformPoint(&vec, &tr.A);
	test_assert(vecEquals(vec, (struct vector){-30.0f, 0.0f, 0.0f}));
	
	return pass;
}

bool transform_scale_Y() {
	bool pass = true;
	struct vector vec = (struct vector){0.0f, -10.0f, 0.0f};
	
	struct transform tr = newTransformScale(1.0f, 3.0f, 1.0f);
	transformPoint(&vec, &tr.A);
	test_assert(vecEquals(vec, (struct vector){0.0f, -30.0f, 0.0f}));
	
	return pass;
}

bool transform_scale_Z() {
	bool pass = true;
	struct vector vec = (struct vector){0.0f, 0.0f, -10.0f};
	
	struct transform tr = newTransformScale(1.0f, 1.0f, 3.0f);
	transformPoint(&vec, &tr.A);
	test_assert(vecEquals(vec, (struct vector){0.0f, 0.0f, -30.0f}));
	
	return pass;
}

bool transform_scale_all() {
	bool pass = true;
	struct vector vec = (struct vector){1.0f, 2.0f, 3.0f};
	
	struct transform tr = newTransformScaleUniform(3.0f);
	transformPoint(&vec, &tr.A);
	test_assert(vecEquals(vec, (struct vector){3.0f, 6.0f, 9.0f}));
	
	return pass;
}

// Inverse
bool transform_inverse() {
	bool pass = true;
	struct matrix4x4 normal = matrixFromParams(0, 0, 0, 1,
											   0, 0, 1, 0,
											   0, 2, 0, 0,
											   2, 0, 0, 0
											);
	struct matrix4x4 inv = inverseMatrix(&normal);
	struct matrix4x4 inv_correct = matrixFromParams(0, 0, 0, 0.5,
													0, 0, 0.5, 0,
													0, 1, 0, 0,
													1, 0, 0, 0);
	test_assert(areMatricesEqual(&inv, &inv_correct));
	return pass;
}

bool matrix_equal() {
	bool pass = true;
	
	struct matrix4x4 A = matrixFromParams(1, 0, 1, 0,
										  0, 1, 0, 1,
										  1, 0, 1, 0,
										  0, 1, 0, 1);
	
	struct matrix4x4 B = matrixFromParams(1, 0, 1, 0,
										  0, 1, 0, 1,
										  1, 0, 1, 0,
										  0, 1, 0, 2);
	
	test_assert(!areMatricesEqual(&A, &B));
	test_assert(areMatricesEqual(&A, &A));
	test_assert(areMatricesEqual(&B, &B));
	
	return pass;
}
