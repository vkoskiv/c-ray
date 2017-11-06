//
//  transforms.h
//  C-Ray
//
//  Created by Valtteri Koskivuori on 07/02/2017.
//  Copyright © 2017 Valtteri Koskivuori. All rights reserved.
//

#pragma once

enum transformType {
	transformTypeXRotate,
	transformTypeYRotate,
	transformTypeZRotate,
	transformTypeTranslate,
	transformTypeScale,
	transformTypeMultiplication,
	transformTypeNone
};

//Reference: http://tinyurl.com/ho6h6mr
struct matrixTransform {
	enum transformType type;
	double a, b, c, d;
	double e, f, g, h;
	double i, j, k, l;
	double m, n, o, p;
};

struct material;

double toRadians(double degrees);

//Transform types
struct matrixTransform newTransformScale(double x, double y, double z);
struct matrixTransform newTransformScaleUniform(double scale);
struct matrixTransform newTransformTranslate(double x, double y, double z);
struct matrixTransform newTransformRotateX(double degrees);
struct matrixTransform newTransformRotateY(double degrees);
struct matrixTransform newTransformRotateZ(double degrees);
struct matrixTransform emptyTransform();

void transformVector(struct vector *vec, struct matrixTransform *tf); //Expose for renderer
