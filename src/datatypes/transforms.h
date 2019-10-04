//
//  transforms.h
//  C-ray
//
//  Created by Valtteri Koskivuori on 07/02/2017.
//  Copyright © 2015-2019 Valtteri Koskivuori. All rights reserved.
//

#pragma once

enum transformType {
	transformTypeXRotate,
	transformTypeYRotate,
	transformTypeZRotate,
	transformTypeTranslate,
	transformTypeScale,
	transformTypeMultiplication,
	transformTypeIdentity,
	transformTypeInverse,
	transformTypeTranspose
};

struct matrix4x4 {
	double mtx[4][4];
};

//Reference: http://tinyurl.com/ho6h6mr
struct transform {
	enum transformType type;
	struct matrix4x4 A;
	struct matrix4x4 Ainv;
};

struct material;

double toRadians(double degrees);
double fromRadians(double radians);

//Transform types
struct transform newTransformScale(double x, double y, double z);
struct transform newTransformScaleUniform(double scale);
struct transform newTransformTranslate(double x, double y, double z);
struct transform newTransformRotateX(double degrees);
struct transform newTransformRotateY(double degrees);
struct transform newTransformRotateZ(double degrees);
struct transform newTransform(void);

struct matrix4x4 inverse(struct matrix4x4 mtx);
struct matrix4x4 transpose(struct matrix4x4 tf);

void transformVector(struct vector *vec, struct matrix4x4 mtx);
