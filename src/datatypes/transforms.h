//
//  transforms.h
//  C-ray
//
//  Created by Valtteri Koskivuori on 07/02/2017.
//  Copyright © 2017-2021 Valtteri Koskivuori. All rights reserved.
//

#pragma once

/*
 C-ray's matrices use a *row-major* notation.
 _______________
 | 00 01 02 03 |
 | 10 11 12 13 |
 | 20 21 22 23 |
 | 30 31 32 33 |
 ---------------
 */

enum transformType {
	transformTypeXRotate,
	transformTypeYRotate,
	transformTypeZRotate,
	transformTypeTranslate,
	transformTypeScale,
	transformTypeMultiplication,
	transformTypeIdentity,
	transformTypeInverse,
	transformTypeTranspose,
	transformTypeComposite
};

struct matrix4x4 {
	float mtx[4][4];
};

//Reference: http://tinyurl.com/ho6h6mr
struct transform {
	enum transformType type;
	struct matrix4x4 A;
	struct matrix4x4 Ainv;
};

struct material;
struct vector;
struct boundingBox;
struct lightRay;

float toRadians(float degrees);
float fromRadians(float radians);

//Transform types
struct transform newTransformScale(float x, float y, float z);
struct transform newTransformScaleUniform(float scale);
struct transform newTransformTranslate(float x, float y, float z);
struct transform newTransformRotateX(float radians);
struct transform newTransformRotateY(float radians);
struct transform newTransformRotateZ(float radians);
struct transform newTransformRotate(float roll, float pitch, float yaw);
struct transform newTransform(void);

struct matrix4x4 inverseMatrix(const float mtx[4][4]);
struct matrix4x4 transposeMatrix(const float mtx[4][4]);
struct matrix4x4 multiplyMatrices(const float A[4][4], const float B[4][4]); //FIXME: Maybe don't expose this.
struct matrix4x4 absoluteMatrix(const float mtx[4][4]);
struct matrix4x4 identityMatrix(void);

void transformPoint(struct vector *vec, const float mtx[4][4]);
void transformVector(struct vector *vec, const float mtx[4][4]);
void transformVectorWithTranspose(struct vector *vec, const float mtx[4][4]);
void transformBBox(struct boundingBox *bbox, const float mtx[4][4]);
void transformRay(struct lightRay *ray, const float mtx[4][4]);

bool isRotation(const struct transform *t);
bool isScale(const struct transform *t);
bool isTranslate(const struct transform *t);

bool areMatricesEqual(const float A[4][4], const float B[4][4]);
