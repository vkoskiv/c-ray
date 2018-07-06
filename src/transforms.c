//
//  transforms.c
//  C-Ray
//
//  Created by Valtteri Koskivuori on 07/02/2017.
//  Copyright © 2017 Valtteri Koskivuori. All rights reserved.
//

#include "includes.h"
#include "transforms.h"

#include "light.h"

//For ease of use
double toRadians(double degrees) {
	return (degrees * PI) / 180;
}

struct matrixTransform emptyTransform() {
	struct matrixTransform transform;
	transform.type = transformTypeNone;
	transform.a = 0;transform.b = 0;transform.c = 0;transform.d = 0;
	transform.e = 0;transform.f = 0;transform.g = 0;transform.h = 0;
	transform.i = 0;transform.j = 0;transform.k = 0;transform.l = 0;
	transform.m = 0;transform.n = 0;transform.o = 0;transform.p = 0;
	return transform;
}

//http://tinyurl.com/hte35pq
void transformVector(struct vector *vec, struct matrixTransform *tf) {
	if (!vec->isTransformed) {
		struct vector temp;
		temp.x = (tf->a * vec->x) + (tf->b * vec->y) + (tf->c * vec->z) + tf->d;
		temp.y = (tf->e * vec->x) + (tf->f * vec->y) + (tf->g * vec->z) + tf->h;
		temp.z = (tf->i * vec->x) + (tf->j * vec->y) + (tf->k * vec->z) + tf->l;
		vec->x = temp.x;
		vec->y = temp.y;
		vec->z = temp.z;
		vec->isTransformed = true;
	}
}

struct matrixTransform newTransformRotateX(double degrees) {
	struct matrixTransform transform = emptyTransform();
	transform.type = transformTypeXRotate;
	transform.a = 1;
	transform.f = cos(toRadians(degrees));
	transform.g = -sin(toRadians(degrees));
	transform.j = sin(toRadians(degrees));
	transform.k = cos(toRadians(degrees));
	transform.p = 1;
	return transform;
}

struct matrixTransform newTransformRotateY(double degrees) {
	struct matrixTransform transform = emptyTransform();
	transform.type = transformTypeYRotate;
	transform.a = cos(toRadians(degrees));
	transform.c = sin(toRadians(degrees));
	transform.f = 1;
	transform.i = -sin(toRadians(degrees));
	transform.k = cos(toRadians(degrees));
	transform.p = 1;
	return transform;
}

struct matrixTransform newTransformRotateZ(double degrees) {
	struct matrixTransform transform = emptyTransform();
	transform.type = transformTypeZRotate;
	transform.a = cos(toRadians(degrees));
	transform.b = -sin(toRadians(degrees));
	transform.e = sin(toRadians(degrees));
	transform.f = cos(toRadians(degrees));
	transform.k = 1;
	transform.p = 1;
	return transform;
}

struct matrixTransform newTransformTranslate(double x, double y, double z) {
	struct matrixTransform transform = emptyTransform();
	transform.type = transformTypeTranslate;
	transform.a = 1;
	transform.f = 1;
	transform.k = 1;
	transform.p = 1;
	transform.d = x;
	transform.h = y;
	transform.l = z;
	return transform;
}

struct matrixTransform newTransformScale(double x, double y, double z) {
	struct matrixTransform transform = emptyTransform();
	transform.type = transformTypeScale;
	transform.a = x;
	transform.f = y;
	transform.k = z;
	transform.p = 1;
	return transform;
}

struct matrixTransform newTransformScaleUniform(double scale) {
	struct matrixTransform transform = emptyTransform();
	transform.type = transformTypeScale;
	transform.a = scale;
	transform.f = scale;
	transform.k = scale;
	transform.p = 1;
	return transform;
}
