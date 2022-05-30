//
//  test_transforms.h
//  C-ray
//
//  Created by Valtteri on 24.6.2020.
//  Copyright © 2020-2021 Valtteri Koskivuori. All rights reserved.
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
	struct matrix4x4 AB = multiplyMatrices(A, B);
	struct matrix4x4 expectedResult = matrixFromParams(
		210, 267, 236, 271,
		93, 149, 104, 149,
		171, 146, 172, 268,
		105, 169, 128, 169);
	test_assert(areMatricesEqual(AB, expectedResult));
	return true;
}

bool transform_transpose() {
	
	// Identity matrices don't care about transpose
	struct matrix4x4 id = identityMatrix();
	struct matrix4x4 tid = transposeMatrix(id);
	test_assert(areMatricesEqual(id, tid));
	
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
	struct matrix4x4 transposed = transposeMatrix(transposable);
	
	test_assert(areMatricesEqual(transposed, expected));
	
	return true;
}

bool transform_determinant() {
	struct matrix4x4 mtx = matrixFromParams(
		1, 2, 0, 0,
		1, 1, 3, 0,
		0, 2, -2, 0,
		0, 0, 3, 1);
	test_assert(findDeterminant(mtx.mtx, 4) == -4.0f);
	return true;
}

bool transform_determinant4x4() {
	struct matrix4x4 mtx = matrixFromParams(
		1, 2, 0, 0,
		1, 1, 3, 0,
		0, 2, -2, 0,
		0, 0, 3, 1);
	test_assert(findDeterminant4x4(mtx.mtx) == -4.0f);
	return true;
}

// Rotations

bool transform_rotate_X() {
	struct transform rotX = newTransformRotateX(toRadians(90.0f));
	struct vector vec = (struct vector){0.0f, 1.0f, 0.0f};
	float original_length = vecLength(vec);
	transformPoint(&vec, rotX.A);
	float new_length = vecLength(vec);
	roughly_equals(original_length, new_length);
	struct vector expected = (struct vector){0.0f, 0.0f, 1.0f};
	vec_roughly_equals(vec, expected);
	return true;
}

bool transform_rotate_Y() {
	struct transform rotY = newTransformRotateY(toRadians(90.0f));
	struct vector vec = (struct vector){1.0f, 0.0f, 0.0f};
	float original_length = vecLength(vec);
	transformPoint(&vec, rotY.A);
	float new_length = vecLength(vec);
	roughly_equals(original_length, new_length);
	struct vector expected = (struct vector){0.0f, 0.0f, -1.0f};
	vec_roughly_equals(vec, expected);
	return true;
}

bool transform_rotate_Z() {
	struct transform rotZ = newTransformRotateZ(toRadians(90.0f));
	struct vector vec = (struct vector){0.0f, 1.0f, 0.0f};
	float original_length = vecLength(vec);
	transformPoint(&vec, rotZ.A);
	float new_length = vecLength(vec);
	roughly_equals(original_length, new_length);
	struct vector expected = (struct vector){-1.0f, 0.0f, 0.0f};
	vec_roughly_equals(vec, expected);
	return true;
}

// Translate

bool transform_translate_X() {
	struct vector vec = (struct vector){-10.0f, 0.0f, 0.0f};
	
	struct transform tr = newTransformTranslate(1.0f, 0.0f, 0.0f);
	transformPoint(&vec, tr.A);
	test_assert(vecEquals(vec, (struct vector){-9.0f, 0.0f, 0.0f}));
	
	return true;
}

bool transform_translate_Y() {
	struct vector vec = (struct vector){0.0f, -10.0f, 0.0f};
	
	struct transform tr = newTransformTranslate(0.0f, 1.0f, 0.0f);
	transformPoint(&vec, tr.A);
	test_assert(vecEquals(vec, (struct vector){0.0f, -9.0f, 0.0f}));
	
	return true;
}

bool transform_translate_Z() {
	struct vector vec = (struct vector){0.0f, 0.0f, -10.0f};
	
	struct transform tr = newTransformTranslate(0.0f, 0.0f, 1.0f);
	transformPoint(&vec, tr.A);
	test_assert(vecEquals(vec, (struct vector){0.0f, 0.0f, -9.0f}));
	
	return true;
}

bool transform_translate_all() {
	struct vector vec = (struct vector){0.0f, 0.0f, 0.0f};
	
	struct transform tr = newTransformTranslate(-1.0f, -10.0f, -100.0f);
	transformPoint(&vec, tr.A);
	test_assert(vecEquals(vec, (struct vector){-1.0f, -10.0f, -100.0f}));
	
	return true;
}


// Scale
bool transform_scale_X() {
	struct vector vec = (struct vector){-10.0f, 0.0f, 0.0f};
	
	struct transform tr = newTransformScale(3.0f, 1.0f, 1.0f);
	transformPoint(&vec, tr.A);
	test_assert(vecEquals(vec, (struct vector){-30.0f, 0.0f, 0.0f}));
	
	return true;
}

bool transform_scale_Y() {
	struct vector vec = (struct vector){0.0f, -10.0f, 0.0f};
	
	struct transform tr = newTransformScale(1.0f, 3.0f, 1.0f);
	transformPoint(&vec, tr.A);
	test_assert(vecEquals(vec, (struct vector){0.0f, -30.0f, 0.0f}));
	
	return true;
}

bool transform_scale_Z() {
	struct vector vec = (struct vector){0.0f, 0.0f, -10.0f};
	
	struct transform tr = newTransformScale(1.0f, 1.0f, 3.0f);
	transformPoint(&vec, tr.A);
	test_assert(vecEquals(vec, (struct vector){0.0f, 0.0f, -30.0f}));
	
	return true;
}

bool transform_scale_all() {
	struct vector vec = (struct vector){1.0f, 2.0f, 3.0f};
	
	struct transform tr = newTransformScaleUniform(3.0f);
	transformPoint(&vec, tr.A);
	test_assert(vecEquals(vec, (struct vector){3.0f, 6.0f, 9.0f}));
	
	return true;
}

// Inverse
bool transform_inverse() {
	struct matrix4x4 normal = matrixFromParams(0, 0, 0, 1,
											   0, 0, 1, 0,
											   0, 2, 0, 0,
											   2, 0, 0, 0
											);
	struct matrix4x4 inv = inverseMatrix(normal);
	struct matrix4x4 inv_correct = matrixFromParams(0, 0, 0, 0.5,
													0, 0, 0.5, 0,
													0, 1, 0, 0,
													1, 0, 0, 0);
	test_assert(areMatricesEqual(inv, inv_correct));
	return true;
}

bool matrix_equal() {
	
	struct matrix4x4 A = matrixFromParams(1, 0, 1, 0,
										  0, 1, 0, 1,
										  1, 0, 1, 0,
										  0, 1, 0, 1);
	
	struct matrix4x4 B = matrixFromParams(1, 0, 1, 0,
										  0, 1, 0, 1,
										  1, 0, 1, 0,
										  0, 1, 0, 2);
	
	test_assert(!areMatricesEqual(A, B));
	test_assert(areMatricesEqual(A, A));
	test_assert(areMatricesEqual(B, B));
	
	return true;
}
