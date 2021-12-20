//
//  transforms.c
//  C-ray
//
//  Created by Valtteri Koskivuori on 07/02/2017.
//  Copyright © 2017-2020 Valtteri Koskivuori. All rights reserved.
//

#include "../includes.h"
#include "transforms.h"

#include "../utils/logging.h"
#include "vector.h"
#include "bbox.h"
#include "lightray.h"
#include "quaternion.h"

//For ease of use
float toRadians(float degrees) {
	return (degrees * PI) / 180.0f;
}

float fromRadians(float radians) {
	return radians * (180.0f / PI);
}

struct matrix4x4 identityMatrix() {
	return (struct matrix4x4) {
		.mtx = {
			{1.0f, 0.0f, 0.0f, 0.0f},
			{0.0f, 1.0f, 0.0f, 0.0f},
			{0.0f, 0.0f, 1.0f, 0.0f},
			{0.0f, 0.0f, 0.0f, 1.0f}
		}
	};
}

struct transform newTransform() {
	struct transform tf;
	tf.type = transformTypeIdentity;
	tf.A = identityMatrix();
	tf.Ainv = tf.A; // Inverse of I == I
	return tf;
}

struct matrix4x4 matrixFromParams(
	float t00, float t01, float t02, float t03,
	float t10, float t11, float t12, float t13,
	float t20, float t21, float t22, float t23,
	float t30, float t31, float t32, float t33) {
	return (struct matrix4x4) {
		.mtx = {
			{t00, t01, t02, t03},
			{t10, t11, t12, t13},
			{t20, t21, t22, t23},
			{t30, t31, t32, t33}
		}
	};
}

struct matrix4x4 absoluteMatrix(const float mtx[4][4]) {
	return (struct matrix4x4) {
		.mtx = {
			// The last column is the translation, and should be kept intact
			{ fabsf(mtx[0][0]), fabsf(mtx[0][1]), fabsf(mtx[0][2]), mtx[0][3] },
			{ fabsf(mtx[1][0]), fabsf(mtx[1][1]), fabsf(mtx[1][2]), mtx[1][3] },
			{ fabsf(mtx[2][0]), fabsf(mtx[2][1]), fabsf(mtx[2][2]), mtx[2][3] },
			// These coefficients are not used anyway
			{ mtx[3][0], mtx[3][1], mtx[3][2], mtx[3][3] }
		}
	};
}

//http://tinyurl.com/hte35pq
//TODO: Boolean switch to inverse, or just feed m4x4 directly
void transformPoint(struct vector *vec, const float mtx[4][4]) {
	struct vector temp;
	temp.x = (mtx[0][0] * vec->x) + (mtx[0][1] * vec->y) + (mtx[0][2] * vec->z) + mtx[0][3];
	temp.y = (mtx[1][0] * vec->x) + (mtx[1][1] * vec->y) + (mtx[1][2] * vec->z) + mtx[1][3];
	temp.z = (mtx[2][0] * vec->x) + (mtx[2][1] * vec->y) + (mtx[2][2] * vec->z) + mtx[2][3];
	*vec = temp;
}

void transformBBox(struct boundingBox *bbox, const float mtx[4][4]) {
	struct matrix4x4 abs = absoluteMatrix(mtx);
	struct vector center = vecScale(vecAdd(bbox->min, bbox->max), 0.5f);
	struct vector halfExtents = vecScale(vecSub(bbox->max, bbox->min), 0.5f);
	transformVector(&halfExtents, abs.mtx);
	transformPoint(&center, abs.mtx);
	bbox->min = vecSub(center, halfExtents);
	bbox->max = vecAdd(center, halfExtents);
}

void transformVector(struct vector *vec, const float mtx[4][4]) {
	struct vector temp;
	temp.x = (mtx[0][0] * vec->x) + (mtx[0][1] * vec->y) + (mtx[0][2] * vec->z);
	temp.y = (mtx[1][0] * vec->x) + (mtx[1][1] * vec->y) + (mtx[1][2] * vec->z);
	temp.z = (mtx[2][0] * vec->x) + (mtx[2][1] * vec->y) + (mtx[2][2] * vec->z);
	*vec = temp;
}

void transformVectorWithTranspose(struct vector *vec, const float mtx[4][4]) {
	// Doing this here gives an opportunity for the compiler
	// to inline the calls to transformVector() and transposeMatrix()
	struct matrix4x4 t = transposeMatrix(mtx);
	transformVector(vec, t.mtx);
}

void transformRay(struct lightRay *ray, const float mtx[4][4]) {
	transformPoint(&ray->start, mtx);
	transformVector(&ray->direction, mtx);
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
	transform.Ainv = inverseMatrix(transform.A.mtx);
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
	transform.Ainv = inverseMatrix(transform.A.mtx);
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
	transform.Ainv = inverseMatrix(transform.A.mtx);
	return transform;
}

// Lifted right off this page here:
// https://www.euclideanspace.com/maths/geometry/rotations/conversions/quaternionToMatrix/index.htm
struct matrix4x4 quaternion_to_matrix(struct quaternion q) {
	const float sqw = q.w * q.w;
	const float sqx = q.x * q.x;
	const float sqy = q.y * q.y;
	const float sqz = q.z * q.z;

	// Inverse square length
	const float invs = 1.0f / (sqx + sqy + sqz + sqw);
	struct matrix4x4 mtx = identityMatrix();
	mtx.mtx[0][0] = ( sqx - sqy - sqz + sqw) * invs;
	mtx.mtx[1][1] = (-sqx + sqy - sqz + sqw) * invs;
	mtx.mtx[2][2] = (-sqx - sqy + sqz + sqw) * invs;
	mtx.mtx[1][0] = 2.0f * ((q.x * q.y) + (q.z * q.w)) * invs;
	mtx.mtx[0][1] = 2.0f * ((q.x * q.y) - (q.z * q.w)) * invs;
	mtx.mtx[2][0] = 2.0f * ((q.x * q.z) - (q.y * q.w)) * invs;
	mtx.mtx[0][2] = 2.0f * ((q.x * q.z) + (q.y * q.w)) * invs;
	mtx.mtx[2][1] = 2.0f * ((q.y * q.z) + (q.x * q.w)) * invs;
	mtx.mtx[1][2] = 2.0f * ((q.y * q.z) - (q.x * q.w)) * invs;

	return mtx;
}

struct transform newTransformRotate(float roll, float pitch, float yaw) {
	struct transform transform;
	transform.type = transformTypeComposite;
	transform.A = quaternion_to_matrix(euler_to_quaternion(roll, pitch, yaw));
	transform.Ainv = inverseMatrix(transform.A.mtx);
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
	transform.Ainv = inverseMatrix(transform.A.mtx);
	return transform;
}

struct transform newTransformScale(float x, float y, float z) {
	ASSERT(x != 0.0f);
	ASSERT(y != 0.0f);
	ASSERT(z != 0.0f);
	struct transform transform = newTransform();
	transform.type = transformTypeScale;
	transform.A.mtx[0][0] = x;
	transform.A.mtx[1][1] = y;
	transform.A.mtx[2][2] = z;
	transform.A.mtx[3][3] = 1.0f;
	transform.Ainv = inverseMatrix(transform.A.mtx);
	return transform;
}

struct transform newTransformScaleUniform(float scale) {
	struct transform transform = newTransform();
	transform.type = transformTypeScale;
	transform.A.mtx[0][0] = scale;
	transform.A.mtx[1][1] = scale;
	transform.A.mtx[2][2] = scale;
	transform.A.mtx[3][3] = 1.0f;
	transform.Ainv = inverseMatrix(transform.A.mtx);
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

struct matrix4x4 inverseMatrix(const float mtx[4][4]) {
	struct matrix4x4 inverse = {{{0}}};
	
	float det = findDeterminant4x4(mtx);
	if (det <= 0.0f) {
		logr(error, "No inverse for given transform!\n");
	}
	
	float adjoint[4][4];
	findAdjoint(mtx, adjoint);
	
	for (int i = 0; i < 4; ++i) {
		for (int j = 0; j < 4; ++j) {
			inverse.mtx[i][j] = adjoint[i][j] / det;
		}
	}
	
	//Not sure why I need to transpose here, but doing so
	//gives correct results
	return transposeMatrix(inverse.mtx);
}

struct matrix4x4 transposeMatrix(const float mtx[4][4]) {
	return matrixFromParams(
		mtx[0][0], mtx[1][0], mtx[2][0], mtx[3][0],
		mtx[0][1], mtx[1][1], mtx[2][1], mtx[3][1],
		mtx[0][2], mtx[1][2], mtx[2][2], mtx[3][2],
		mtx[0][3], mtx[1][3], mtx[2][3], mtx[3][3]
	);
}

struct matrix4x4 multiplyMatrices(const float A[4][4], const float B[4][4]) {
	return matrixFromParams(
		A[0][0] * B[0][0] + A[0][1] * B[1][0] + A[0][2] * B[2][0] + A[0][3] * B[3][0],
		A[0][0] * B[0][1] + A[0][1] * B[1][1] + A[0][2] * B[2][1] + A[0][3] * B[3][1],
		A[0][0] * B[0][2] + A[0][1] * B[1][2] + A[0][2] * B[2][2] + A[0][3] * B[3][2],
		A[0][0] * B[0][3] + A[0][1] * B[1][3] + A[0][2] * B[2][3] + A[0][3] * B[3][3],

		A[1][0] * B[0][0] + A[1][1] * B[1][0] + A[1][2] * B[2][0] + A[1][3] * B[3][0],
		A[1][0] * B[0][1] + A[1][1] * B[1][1] + A[1][2] * B[2][1] + A[1][3] * B[3][1],
		A[1][0] * B[0][2] + A[1][1] * B[1][2] + A[1][2] * B[2][2] + A[1][3] * B[3][2],
		A[1][0] * B[0][3] + A[1][1] * B[1][3] + A[1][2] * B[2][3] + A[1][3] * B[3][3],

		A[2][0] * B[0][0] + A[2][1] * B[1][0] + A[2][2] * B[2][0] + A[2][3] * B[3][0],
		A[2][0] * B[0][1] + A[2][1] * B[1][1] + A[2][2] * B[2][1] + A[2][3] * B[3][1],
		A[2][0] * B[0][2] + A[2][1] * B[1][2] + A[2][2] * B[2][2] + A[2][3] * B[3][2],
		A[2][0] * B[0][3] + A[2][1] * B[1][3] + A[2][2] * B[2][3] + A[2][3] * B[3][3],

		A[3][0] * B[0][0] + A[3][1] * B[1][0] + A[3][2] * B[2][0] + A[3][3] * B[3][0],
		A[3][0] * B[0][1] + A[3][1] * B[1][1] + A[3][2] * B[2][1] + A[3][3] * B[3][1],
		A[3][0] * B[0][2] + A[3][1] * B[1][2] + A[3][2] * B[2][2] + A[3][3] * B[3][2],
		A[3][0] * B[0][3] + A[3][1] * B[1][3] + A[3][2] * B[2][3] + A[3][3] * B[3][3]);
}

bool isRotation(const struct transform *t) {
	return (t->type == transformTypeXRotate) || (t->type == transformTypeYRotate) || (t->type == transformTypeZRotate);
}

bool isScale(const struct transform *t) {
	return t->type == transformTypeScale;
}

bool isTranslate(const struct transform *t) {
	return t->type == transformTypeTranslate;
}

bool areMatricesEqual(const float A[4][4], const float B[4][4]) {
	for (unsigned j = 0; j < 4; ++j) {
		for (unsigned i = 0; i < 4; ++i) {
			if (A[i][j] != B[i][j]) return false;
		}
	}
	return true;
}
