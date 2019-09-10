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
	transformTypeNone,
	transformTypeInverse,
	transformTypeTranspose
};

//Reference: http://tinyurl.com/ho6h6mr
struct transform {
	enum transformType type;
	double A[4][4];
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
struct transform emptyTransform(void);

struct transform inverse(struct transform tf);
struct transform transpose(struct transform tf);

void transformVector(struct vector *vec, struct transform *tf);
