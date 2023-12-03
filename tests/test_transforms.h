//
//  test_transforms.h
//  C-ray
//
//  Created by Valtteri on 24.6.2020.
//  Copyright © 2020-2021 Valtteri Koskivuori. All rights reserved.
//

#include "../src/common/transforms.h"
#include "../src/common/vector.h"

// Grab private functions
float findDeterminant(float A[4][4], int n);
float findDeterminant4x4(float A[4][4]);

struct matrix4x4 mat_id(void);

bool transform_multiply() {
	struct matrix4x4 A = (struct matrix4x4){
		.mtx = {
			{ 5,  7, 9, 10 },
			{ 2,  3, 3,  8 },
			{ 8, 10, 2,  3 },
			{ 3,  3, 4,  8 },
		}
	};
	struct matrix4x4 B = (struct matrix4x4){
		.mtx = {
			{  3, 10, 12, 18 },
			{ 12,  1,  4,  9 },
			{  9, 10, 12,  2 },
			{  3, 12,  4, 10 },
		}
	};
	struct matrix4x4 AB = mat_mul(A, B);
	struct matrix4x4 expectedResult = (struct matrix4x4){
		.mtx = {
			{ 210, 267, 236, 271 },
			{  93, 149, 104, 149 },
			{ 171, 146, 172, 268 },
			{ 105, 169, 128, 169 },
		}
	};
	test_assert(mat_eq(AB, expectedResult));
	return true;
}

bool transform_transpose() {
	
	// Identity matrices don't care about transpose
	struct matrix4x4 id = mat_id();
	struct matrix4x4 tid = mat_transpose(id);
	test_assert(mat_eq(id, tid));
	
	struct matrix4x4 transposable = (struct matrix4x4){
		.mtx = {
			{ 0, 0, 0, 1 },
			{ 0, 0, 1, 0 },
			{ 0, 2, 0, 0 },
			{ 2, 0, 0, 0 },
		}
	};
	
	struct matrix4x4 expected = (struct matrix4x4){
		.mtx = {
			{ 0, 0, 0, 2 },
			{ 0, 0, 2, 0 },
			{ 0, 1, 0, 0 },
			{ 1, 0, 0, 0 },
		}
	};
	struct matrix4x4 transposed = mat_transpose(transposable);
	
	test_assert(mat_eq(transposed, expected));
	
	return true;
}

bool transform_determinant() {
	struct matrix4x4 mtx = (struct matrix4x4){
		.mtx = {
			{ 1, 2, 0, 0 },
			{ 1, 1, 3, 0 },
			{ 0, 2, -2, 0 },
			{ 0, 0, 3, 1 },
		}
	};
	test_assert(findDeterminant(mtx.mtx, 4) == -4.0f);
	return true;
}

bool transform_determinant4x4() {
	struct matrix4x4 mtx = (struct matrix4x4){
		.mtx = {
			{ 1, 2, 0, 0 },
			{ 1, 1, 3, 0 },
			{ 0, 2, -2, 0 },
			{ 0, 0, 3, 1 },
		}
	};
	test_assert(findDeterminant4x4(mtx.mtx) == -4.0f);
	return true;
}

// Rotations

bool transform_rotate_X() {
	struct transform rotX = tform_new_rot_x(deg_to_rad(90.0f));
	struct vector vec = (struct vector){0.0f, 1.0f, 0.0f};
	float original_length = vec_length(vec);
	tform_point(&vec, rotX.A);
	float new_length = vec_length(vec);
	roughly_equals(original_length, new_length);
	struct vector expected = (struct vector){0.0f, 0.0f, 1.0f};
	vec_roughly_equals(vec, expected);
	return true;
}

bool transform_rotate_Y() {
	struct transform rotY = tform_new_rot_y(deg_to_rad(90.0f));
	struct vector vec = (struct vector){1.0f, 0.0f, 0.0f};
	float original_length = vec_length(vec);
	tform_point(&vec, rotY.A);
	float new_length = vec_length(vec);
	roughly_equals(original_length, new_length);
	struct vector expected = (struct vector){0.0f, 0.0f, -1.0f};
	vec_roughly_equals(vec, expected);
	return true;
}

bool transform_rotate_Z() {
	struct transform rotZ = tform_new_rot_z(deg_to_rad(90.0f));
	struct vector vec = (struct vector){0.0f, 1.0f, 0.0f};
	float original_length = vec_length(vec);
	tform_point(&vec, rotZ.A);
	float new_length = vec_length(vec);
	roughly_equals(original_length, new_length);
	struct vector expected = (struct vector){-1.0f, 0.0f, 0.0f};
	vec_roughly_equals(vec, expected);
	return true;
}

// Translate

bool transform_translate_X() {
	struct vector vec = (struct vector){-10.0f, 0.0f, 0.0f};
	
	struct transform tr = tform_new_translate(1.0f, 0.0f, 0.0f);
	tform_point(&vec, tr.A);
	test_assert(vec_equals(vec, (struct vector){-9.0f, 0.0f, 0.0f}));
	
	return true;
}

bool transform_translate_Y() {
	struct vector vec = (struct vector){0.0f, -10.0f, 0.0f};
	
	struct transform tr = tform_new_translate(0.0f, 1.0f, 0.0f);
	tform_point(&vec, tr.A);
	test_assert(vec_equals(vec, (struct vector){0.0f, -9.0f, 0.0f}));
	
	return true;
}

bool transform_translate_Z() {
	struct vector vec = (struct vector){0.0f, 0.0f, -10.0f};
	
	struct transform tr = tform_new_translate(0.0f, 0.0f, 1.0f);
	tform_point(&vec, tr.A);
	test_assert(vec_equals(vec, (struct vector){0.0f, 0.0f, -9.0f}));
	
	return true;
}

bool transform_translate_all() {
	struct vector vec = (struct vector){0.0f, 0.0f, 0.0f};
	
	struct transform tr = tform_new_translate(-1.0f, -10.0f, -100.0f);
	tform_point(&vec, tr.A);
	test_assert(vec_equals(vec, (struct vector){-1.0f, -10.0f, -100.0f}));
	
	return true;
}


// Scale
bool transform_scale_X() {
	struct vector vec = (struct vector){-10.0f, 0.0f, 0.0f};
	
	struct transform tr = tform_new_scale3(3.0f, 1.0f, 1.0f);
	tform_point(&vec, tr.A);
	test_assert(vec_equals(vec, (struct vector){-30.0f, 0.0f, 0.0f}));
	
	return true;
}

bool transform_scale_Y() {
	struct vector vec = (struct vector){0.0f, -10.0f, 0.0f};
	
	struct transform tr = tform_new_scale3(1.0f, 3.0f, 1.0f);
	tform_point(&vec, tr.A);
	test_assert(vec_equals(vec, (struct vector){0.0f, -30.0f, 0.0f}));
	
	return true;
}

bool transform_scale_Z() {
	struct vector vec = (struct vector){0.0f, 0.0f, -10.0f};
	
	struct transform tr = tform_new_scale3(1.0f, 1.0f, 3.0f);
	tform_point(&vec, tr.A);
	test_assert(vec_equals(vec, (struct vector){0.0f, 0.0f, -30.0f}));
	
	return true;
}

bool transform_scale_all() {
	struct vector vec = (struct vector){1.0f, 2.0f, 3.0f};
	
	struct transform tr = tform_new_scale(3.0f);
	tform_point(&vec, tr.A);
	test_assert(vec_equals(vec, (struct vector){3.0f, 6.0f, 9.0f}));
	
	return true;
}

// Inverse
bool transform_inverse() {
	struct matrix4x4 normal = (struct matrix4x4){
		.mtx = {
			{ 0, 0, 0, 1 },
			{ 0, 0, 1, 0 },
			{ 0, 2, 0, 0 },
			{ 2, 0, 0, 0 },
		}
	};
	struct matrix4x4 inv = mat_invert(normal);
	struct matrix4x4 inv_correct = (struct matrix4x4){
		.mtx = {
			{ 0, 0,   0, 0.5 },
			{ 0, 0, 0.5,   0 },
			{ 0, 1,   0,   0 },
			{ 1, 0,   0,   0 },
		}
	};
	test_assert(mat_eq(inv, inv_correct));
	return true;
}

bool matrix_equal() {
	
	struct matrix4x4 A = (struct matrix4x4){
		.mtx = {
			{ 1, 0, 1, 0 },
			{ 0, 1, 0, 1 },
			{ 1, 0, 1, 0 },
			{ 0, 1, 0, 1 },
		}
	};
	
	struct matrix4x4 B = (struct matrix4x4){
		.mtx = {
			{ 1, 0, 1, 0 },
			{ 0, 1, 0, 1 },
			{ 1, 0, 1, 0 },
			{ 0, 1, 0, 2 },
		}
	};
	
	test_assert(!mat_eq(A, B));
	test_assert(mat_eq(A, A));
	test_assert(mat_eq(B, B));
	
	return true;
}
