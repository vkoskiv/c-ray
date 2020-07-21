//
//  transforms.h
//  C-ray
//
//  Created by Valtteri Koskivuori on 07/02/2017.
//  Copyright © 2015-2020 Valtteri Koskivuori. All rights reserved.
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
	struct matrix4x4 AinvT;
};

struct material;
struct vector;
struct boundingBox;

float toRadians(float degrees);
float fromRadians(float radians);

//Transform types
struct transform newTransformScale(float x, float y, float z);
struct transform newTransformScaleUniform(float scale);
struct transform newTransformTranslate(float x, float y, float z);
struct transform newTransformRotateX(float radians);
struct transform newTransformRotateY(float radians);
struct transform newTransformRotateZ(float radians);
struct transform newTransform(void);

struct matrix4x4 inverse(struct matrix4x4 mtx);
struct matrix4x4 transpose(struct matrix4x4 tf);
struct matrix4x4 multiply(struct matrix4x4 A, struct matrix4x4 B); //FIXME: Maybe don't expose this.

void transformPoint(struct vector *vec, struct matrix4x4 mtx);
void transformVector(struct vector *vec, struct matrix4x4 mtx);
void transformBBox(struct boundingBox *bbox, struct matrix4x4 *mtx);

bool isRotation(struct transform t);
bool isScale(struct transform t);
bool isTranslate(struct transform t);
