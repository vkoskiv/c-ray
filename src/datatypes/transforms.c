//
//  transforms.c
//  C-ray
//
//  Created by Valtteri Koskivuori on 07/02/2017.
//  Copyright © 2015-2020 Valtteri Koskivuori. All rights reserved.
//

#include "../includes.h"
#include "transforms.h"

#include "../utils/logging.h"
#include "vector.h"
#include "bbox.h"

//For ease of use
float toRadians(float degrees) {
	return (degrees * PI) / 180.0f;
}

float fromRadians(float radians) {
	return radians * (180.0f / PI);
}

struct matrix4x4 identityMatrix() {
	struct matrix4x4 mtx;
	for (int i = 0; i < 4; ++i) {
		for (int j = 0; j < 4; ++j) {
			mtx.mtx[i][j] = 0.0f;
		}
	}
	mtx.mtx[0][0] = 1.0f;
	mtx.mtx[1][1] = 1.0f;
	mtx.mtx[2][2] = 1.0f;
	mtx.mtx[3][3] = 1.0f;
	return mtx;
}

struct transform newTransform() {
	struct transform tf;
	tf.type = transformTypeIdentity;
	tf.A = identityMatrix();
	tf.Ainv = tf.A; // Inverse of I == I
	return tf;
}

struct matrix4x4 fromParams(float t00, float t01, float t02, float t03,
							float t10, float t11, float t12, float t13,
							float t20, float t21, float t22, float t23,
							float t30, float t31, float t32, float t33) {
	struct matrix4x4 new = {{{0}}};
	new.mtx[0][0] = t00; new.mtx[0][1] = t01; new.mtx[0][2] = t02; new.mtx[0][3] = t03;
	new.mtx[1][0] = t10; new.mtx[1][1] = t11; new.mtx[1][2] = t12; new.mtx[1][3] = t13;
	new.mtx[2][0] = t20; new.mtx[2][1] = t21; new.mtx[2][2] = t22; new.mtx[2][3] = t23;
	new.mtx[3][0] = t30; new.mtx[3][1] = t31; new.mtx[3][2] = t32; new.mtx[3][3] = t33;
	return new;
}

//http://tinyurl.com/hte35pq
//TODO: Boolean switch to inverse, or just feed m4x4 directly
void transformPoint(struct vector *vec, struct matrix4x4 mtx) {
	struct vector temp;
	temp.x = (mtx.mtx[0][0] * vec->x) + (mtx.mtx[0][1] * vec->y) + (mtx.mtx[0][2] * vec->z) + mtx.mtx[0][3];
	temp.y = (mtx.mtx[1][0] * vec->x) + (mtx.mtx[1][1] * vec->y) + (mtx.mtx[1][2] * vec->z) + mtx.mtx[1][3];
	temp.z = (mtx.mtx[2][0] * vec->x) + (mtx.mtx[2][1] * vec->y) + (mtx.mtx[2][2] * vec->z) + mtx.mtx[2][3];
	vec->x = temp.x;
	vec->y = temp.y;
	vec->z = temp.z;
}

void transformBBox(struct boundingBox *bbox, struct matrix4x4 *mtx) {
	transformPoint(&bbox->max, *mtx);
	transformPoint(&bbox->min, *mtx);
}

void transformVector(struct vector *vec, struct matrix4x4 mtx) {
	struct vector temp;
	temp.x = (mtx.mtx[0][0] * vec->x) + (mtx.mtx[0][1] * vec->y) + (mtx.mtx[0][2] * vec->z);
	temp.y = (mtx.mtx[1][0] * vec->x) + (mtx.mtx[1][1] * vec->y) + (mtx.mtx[1][2] * vec->z);
	temp.z = (mtx.mtx[2][0] * vec->x) + (mtx.mtx[2][1] * vec->y) + (mtx.mtx[2][2] * vec->z);
	vec->x = temp.x;
	vec->y = temp.y;
	vec->z = temp.z;
}

struct transform newTransformRotateX(float rads) {
	struct transform transform = newTransform();
	transform.type = transformTypeXRotate;
	float cosRads = cosf(rads);
	float sinRads = sinf(rads);
	transform.A.mtx[0][0] = 1.0f;
	transform.A.mtx[1][1] = cosRads;
	transform.A.mtx[1][2] = -sinRads;
	transform.A.mtx[2][1] = sinRads;
	transform.A.mtx[2][2] = cosRads;
	transform.A.mtx[3][3] = 1.0f;
	transform.Ainv = inverse(transform.A);
	transform.AinvT = transpose(transform.Ainv);
	return transform;
}

struct transform newTransformRotateY(float rads) {
	struct transform transform = newTransform();
	transform.type = transformTypeYRotate;
	float cosRads = cosf(rads);
	float sinRads = sinf(rads);
	transform.A.mtx[0][0] = cosRads;
	transform.A.mtx[0][2] = sinRads;
	transform.A.mtx[1][1] = 1.0f;
	transform.A.mtx[2][0] = -sinRads;
	transform.A.mtx[2][2] = cosRads;
	transform.A.mtx[3][3] = 1.0f;
	transform.Ainv = inverse(transform.A);
	transform.AinvT = transpose(transform.Ainv);
	return transform;
}

struct transform newTransformRotateZ(float rads) {
	struct transform transform = newTransform();
	transform.type = transformTypeZRotate;
	float cosRads = cosf(rads);
	float sinRads = sinf(rads);
	transform.A.mtx[0][0] = cosRads;
	transform.A.mtx[0][1] = -sinRads;
	transform.A.mtx[1][0] = sinRads;
	transform.A.mtx[1][1] = cosRads;
	transform.A.mtx[2][2] = 1.0f;
	transform.A.mtx[3][3] = 1.0f;
	transform.Ainv = inverse(transform.A);
	transform.AinvT = transpose(transform.Ainv);
	return transform;
}

struct transform newTransformTranslate(float x, float y, float z) {
	struct transform transform = newTransform();
	transform.type = transformTypeTranslate;
	transform.A.mtx[0][0] = 1.0f;
	transform.A.mtx[1][1] = 1.0f;
	transform.A.mtx[2][2] = 1.0f;
	transform.A.mtx[3][3] = 1.0f;
	transform.A.mtx[0][3] = x;
	transform.A.mtx[1][3] = y;
	transform.A.mtx[2][3] = z;
	transform.Ainv = inverse(transform.A);
	transform.AinvT = transpose(transform.Ainv);
	return transform;
}

struct transform newTransformScale(float x, float y, float z) {
	struct transform transform = newTransform();
	transform.type = transformTypeScale;
	transform.A.mtx[0][0] = x;
	transform.A.mtx[1][1] = y;
	transform.A.mtx[2][2] = z;
	transform.A.mtx[3][3] = 1.0f;
	transform.Ainv = inverse(transform.A);
	transform.AinvT = transpose(transform.Ainv);
	return transform;
}

struct transform newTransformScaleUniform(float scale) {
	struct transform transform = newTransform();
	transform.type = transformTypeScale;
	transform.A.mtx[0][0] = scale;
	transform.A.mtx[1][1] = scale;
	transform.A.mtx[2][2] = scale;
	transform.A.mtx[3][3] = 1.0f;
	transform.Ainv = inverse(transform.A);
	transform.AinvT = transpose(transform.Ainv);
	return transform;
}

static void getCofactor(const float A[4][4], float cofactors[4][4], int p, int q, int n) {
	int i = 0;
	int j = 0;
	
	for (int row = 0; row < n; ++row) {
		for (int col = 0; col < n; ++col) {
			if (row != p && col != q) {
				cofactors[i][j++] = A[row][col];
				if (j == n - 1) {
					j = 0;
					i++;
				}
			}
		}
	}
}

//I really, really hope this is faster than the generic one
//I wrote this by hand...
float findDeterminant4x4(const float A[4][4]) {
	float topLeft = A[0][0] * ((A[1][1] * ((A[2][2]*A[3][3])-(A[2][3]*A[3][2]))) - (A[1][2] * ((A[2][1]*A[3][3])-(A[2][3]*A[3][1]))) + (A[1][3] * ((A[2][1]*A[3][2])-(A[2][2]*A[3][1]))));
	float topRigh = A[0][1] * ((A[1][0] * ((A[2][2]*A[3][3])-(A[2][3]*A[3][2]))) - (A[1][2] * ((A[2][0]*A[3][3])-(A[2][3]*A[3][0]))) + (A[1][3] * ((A[2][0]*A[3][2])-(A[2][2]*A[3][0]))));
	float botLeft = A[0][2] * ((A[1][0] * ((A[2][1]*A[3][3])-(A[2][3]*A[3][1]))) - (A[1][1] * ((A[2][0]*A[3][3])-(A[2][3]*A[3][0]))) + (A[1][3] * ((A[2][0]*A[3][1])-(A[2][1]*A[3][0]))));
	float botRigh = A[0][3] * ((A[1][0] * ((A[2][1]*A[3][2])-(A[2][2]*A[3][1]))) - (A[1][1] * ((A[2][0]*A[3][2])-(A[2][2]*A[3][0]))) + (A[1][2] * ((A[2][0]*A[3][1])-(A[2][1]*A[3][0]))));
	return topLeft - topRigh + botLeft - botRigh;
}

//Find det of a given 4x4 matrix A
float findDeterminant(float A[4][4], int n) {
	float det = 0.0f;
	
	if (n == 1)
		return A[0][0];
	
	float cofactors[4][4];
	float sign = 1.0f;
	
	for (int f = 0; f < n; ++f) {
		getCofactor(A, cofactors, 0, f, n);
		det += sign * A[0][f] * findDeterminant(cofactors, n - 1);
		sign = -sign;
	}
	
	return det;
}

static void findAdjoint(const float A[4][4], float adjoint[4][4]) {
	int sign = 1;
	float temp[4][4];
	
	for (int i = 0; i < 4; ++i) {
		for (int j = 0; j < 4; ++j) {
			getCofactor(A, temp, i, j, 4);
			sign = ((i+j)%2 == 0) ? 1 : -1;
			adjoint[i][j] = (sign)*(findDeterminant(temp, 3));
		}
	}
}

struct matrix4x4 inverse(const struct matrix4x4 mtx) {
	struct matrix4x4 inverse = {{{0}}};
	
	float det = findDeterminant4x4(mtx.mtx);
	if (det <= 0.0f) {
		logr(error, "No inverse for given transform!\n");
	}
	
	float adjoint[4][4];
	findAdjoint(mtx.mtx, adjoint);
	
	for (int i = 0; i < 4; ++i) {
		for (int j = 0; j < 4; ++j) {
			inverse.mtx[i][j] = adjoint[i][j] / det;
		}
	}
	
	//Not sure why I need to transpose here, but doing so
	//gives correct results
	return transpose(inverse);
}

struct matrix4x4 transpose(struct matrix4x4 tf) {
	return fromParams(
					tf.mtx[0][0], tf.mtx[1][0], tf.mtx[2][0], tf.mtx[3][0],
					tf.mtx[0][1], tf.mtx[1][1], tf.mtx[2][1], tf.mtx[3][1],
					tf.mtx[0][2], tf.mtx[1][2], tf.mtx[2][2], tf.mtx[3][2],
					tf.mtx[0][3], tf.mtx[1][3], tf.mtx[2][3], tf.mtx[3][3]
					);
}

struct matrix4x4 multiply(struct matrix4x4 A, struct matrix4x4 B) {
	return fromParams(
					A.mtx[0][0] * B.mtx[0][0] + A.mtx[0][1] * B.mtx[1][0] + A.mtx[0][2] * B.mtx[2][0] + A.mtx[0][3] * B.mtx[3][0],
					A.mtx[0][0] * B.mtx[0][1] + A.mtx[0][1] * B.mtx[1][1] + A.mtx[0][2] * B.mtx[2][1] + A.mtx[0][3] * B.mtx[3][1],
					A.mtx[0][0] * B.mtx[0][2] + A.mtx[0][1] * B.mtx[1][2] + A.mtx[0][2] * B.mtx[2][2] + A.mtx[0][3] * B.mtx[3][2],
					A.mtx[0][0] * B.mtx[0][3] + A.mtx[0][1] * B.mtx[1][3] + A.mtx[0][2] * B.mtx[2][3] + A.mtx[0][3] * B.mtx[3][3],
					
					A.mtx[1][0] * B.mtx[0][0] + A.mtx[1][1] * B.mtx[1][0] + A.mtx[1][2] * B.mtx[2][0] + A.mtx[1][3] * B.mtx[3][0],
					A.mtx[1][0] * B.mtx[0][1] + A.mtx[1][1] * B.mtx[1][1] + A.mtx[1][2] * B.mtx[2][1] + A.mtx[1][3] * B.mtx[3][1],
					A.mtx[1][0] * B.mtx[0][2] + A.mtx[1][1] * B.mtx[1][2] + A.mtx[1][2] * B.mtx[2][2] + A.mtx[1][3] * B.mtx[3][2],
					A.mtx[1][0] * B.mtx[0][3] + A.mtx[1][1] * B.mtx[1][3] + A.mtx[1][2] * B.mtx[2][3] + A.mtx[1][3] * B.mtx[3][3],
					
					A.mtx[2][0] * B.mtx[0][0] + A.mtx[2][1] * B.mtx[1][0] + A.mtx[2][2] * B.mtx[2][0] + A.mtx[2][3] * B.mtx[3][0],
					A.mtx[2][0] * B.mtx[0][1] + A.mtx[2][1] * B.mtx[1][1] + A.mtx[2][2] * B.mtx[2][1] + A.mtx[2][3] * B.mtx[3][1],
					A.mtx[2][0] * B.mtx[0][2] + A.mtx[2][1] * B.mtx[1][2] + A.mtx[2][2] * B.mtx[2][2] + A.mtx[2][3] * B.mtx[3][2],
					A.mtx[2][0] * B.mtx[0][3] + A.mtx[2][1] * B.mtx[1][3] + A.mtx[2][2] * B.mtx[2][3] + A.mtx[2][3] * B.mtx[3][3],
					
					A.mtx[3][0] * B.mtx[0][0] + A.mtx[3][1] * B.mtx[1][0] + A.mtx[3][2] * B.mtx[2][0] + A.mtx[3][3] * B.mtx[3][0],
					A.mtx[3][0] * B.mtx[0][1] + A.mtx[3][1] * B.mtx[1][1] + A.mtx[3][2] * B.mtx[2][1] + A.mtx[3][3] * B.mtx[3][1],
					A.mtx[3][0] * B.mtx[0][2] + A.mtx[3][1] * B.mtx[1][2] + A.mtx[3][2] * B.mtx[2][2] + A.mtx[3][3] * B.mtx[3][2],
					A.mtx[3][0] * B.mtx[0][3] + A.mtx[3][1] * B.mtx[1][3] + A.mtx[3][2] * B.mtx[2][3] + A.mtx[3][3] * B.mtx[3][3]
			);
}

static char *transformTypeString(enum transformType type) {
	switch (type) {
		case transformTypeXRotate:
			return "transformTypeXRotate";
		case transformTypeYRotate:
			return "transformTypeYRotate";
		case transformTypeZRotate:
			return "transformTypeZRotate";
		case transformTypeTranslate:
			return "transformTypeTranslate";
		case transformTypeScale:
			return "transformTypeScale";
		case transformTypeMultiplication:
			return "transformTypeMultiplication";
		case transformTypeIdentity:
			return "transformTypeIdentity";
		case transformTypeInverse:
			return "transformTypeInverse";
		case transformTypeTranspose:
			return "transformTypeTranspose";
		case transformTypeComposite:
			return "transformTypeComposite";
		default:
			return "unknownTransform";
	}
}

bool isRotation(struct transform t) {
	return (t.type == transformTypeXRotate) || (t.type == transformTypeYRotate) || (t.type == transformTypeZRotate);
}

bool isScale(struct transform t) {
	return t.type == transformTypeScale;
}

bool isTranslate(struct transform t) {
	return t.type == transformTypeTranslate;
}

static void printMatrix(struct matrix4x4 mtx) {
	printf("\n");
	for (int i = 0; i < 4; ++i) {
		for (int j = 0; j < 4; ++j) {
			printf("mtx.mtx[%i][%i]=%s%f ",i,j, mtx.mtx[i][j] < 0 ? "" : " " ,mtx.mtx[i][j]);
		}
		printf("\n");
	}
	printf("\n");
}

/*static void printTransform(struct transform tf) {
	printf("%s", transformTypeString(tf.type));
	printMatrix(tf.A);
	printMatrix(tf.Ainv);
}*/

bool areEqual(struct matrix4x4 A, struct matrix4x4 B) {
	bool matches = true;
	for (unsigned j = 0; j < 4; ++j) {
		for (unsigned i = 0; i < 4; ++i) {
			if (A.mtx[i][j] != B.mtx[i][j]) matches = false;
		}
	}
	return matches;
}
