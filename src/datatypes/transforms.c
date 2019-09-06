//
//  transforms.c
//  C-ray
//
//  Created by Valtteri Koskivuori on 07/02/2017.
//  Copyright © 2015-2019 Valtteri Koskivuori. All rights reserved.
//

#include "../includes.h"
#include "transforms.h"

#include "../utils/logging.h"

//For ease of use
double toRadians(double degrees) {
	return (degrees * PI) / 180;
}

double fromRadians(double radians) {
	return radians * (180/PI);
}

struct transform emptyTransform() {
	struct transform transform;
	transform.type = transformTypeNone;
	transform.a = 0;transform.b = 0;transform.c = 0;transform.d = 0;
	transform.e = 0;transform.f = 0;transform.g = 0;transform.h = 0;
	transform.i = 0;transform.j = 0;transform.k = 0;transform.l = 0;
	transform.m = 0;transform.n = 0;transform.o = 0;transform.p = 0;
	return transform;
}

//http://tinyurl.com/hte35pq
void transformVector(struct vector *vec, struct transform *tf) {
	struct vector temp;
	temp.x = (tf->a * vec->x) + (tf->b * vec->y) + (tf->c * vec->z) + tf->d;
	temp.y = (tf->e * vec->x) + (tf->f * vec->y) + (tf->g * vec->z) + tf->h;
	temp.z = (tf->i * vec->x) + (tf->j * vec->y) + (tf->k * vec->z) + tf->l;
	vec->x = temp.x;
	vec->y = temp.y;
	vec->z = temp.z;
}

struct transform newTransformRotateX(double degrees) {
	struct transform transform = emptyTransform();
	double rads = toRadians(degrees);
	transform.type = transformTypeXRotate;
	transform.a = 1;
	transform.f = cos(rads);
	transform.g = -sin(rads);
	transform.j = sin(rads);
	transform.k = cos(rads);
	transform.p = 1;
	return transform;
}

struct transform newTransformRotateY(double degrees) {
	struct transform transform = emptyTransform();
	double rads = toRadians(degrees);
	transform.type = transformTypeYRotate;
	transform.a = cos(rads);
	transform.c = sin(rads);
	transform.f = 1;
	transform.i = -sin(rads);
	transform.k = cos(rads);
	transform.p = 1;
	return transform;
}

struct transform newTransformRotateZ(double degrees) {
	struct transform transform = emptyTransform();
	double rads = toRadians(degrees);
	transform.type = transformTypeZRotate;
	transform.a = cos(rads);
	transform.b = -sin(rads);
	transform.e = sin(rads);
	transform.f = cos(rads);
	transform.k = 1;
	transform.p = 1;
	return transform;
}

struct transform newTransformTranslate(double x, double y, double z) {
	struct transform transform = emptyTransform();
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

struct transform newTransformScale(double x, double y, double z) {
	struct transform transform = emptyTransform();
	transform.type = transformTypeScale;
	transform.a = x;
	transform.f = y;
	transform.k = z;
	transform.p = 1;
	return transform;
}

struct transform newTransformScaleUniform(double scale) {
	struct transform transform = emptyTransform();
	transform.type = transformTypeScale;
	transform.a = scale;
	transform.f = scale;
	transform.k = scale;
	transform.p = 1;
	return transform;
}

void transferToMatrix(double A[4][4], struct transform tf) {
	A[0][0] = tf.a;A[0][1] = tf.b;A[0][2] = tf.c;A[0][3] = tf.d;
	A[1][0] = tf.e;A[1][1] = tf.f;A[1][2] = tf.g;A[1][3] = tf.h;
	A[2][0] = tf.i;A[2][1] = tf.j;A[2][2] = tf.k;A[2][3] = tf.l;
	A[3][0] = tf.m;A[3][1] = tf.n;A[3][2] = tf.o;A[3][3] = tf.p;
}

void transferFromMatrix(struct transform *tf, double A[4][4]) {
	tf->a = A[0][0];tf->b = A[0][1];tf->c = A[0][2];tf->d = A[0][3];
	tf->e = A[1][0];tf->f = A[1][1];tf->g = A[1][2];tf->h = A[1][3];
	tf->i = A[2][0];tf->j = A[2][1];tf->k = A[2][2];tf->l = A[2][3];
	tf->m = A[3][0];tf->n = A[3][1];tf->o = A[3][2];tf->p = A[3][3];
}

void getCofactor(double A[4][4], double cofactors[4][4], int p, int q, int n) {
	int i = 0;
	int j = 0;
	
	for (int row = 0; row < n; row++) {
		for (int col = 0; col < n; col++) {
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

//Find det of a given 4x4 matrix A
double findDeterminant(double A[4][4], int n) {
	double det = 0;
	
	if (n == 1)
		return A[0][0];
	
	double cofactors[4][4];
	int sign = 1;
	
	for (int f = 0; f < n; f++) {
		getCofactor(A, cofactors, 0, f, n);
		det += sign * A[0][f] * findDeterminant(cofactors, n - 1);
		sign = -sign;
	}
	
	return det;
}

void findAdjoint(double A[4][4], double adjoint[4][4]) {
	int sign = 1;
	double temp[4][4];
	
	for (int i = 0; i < 4; i++) {
		for (int j = 0; j < 4; j++) {
			getCofactor(A, temp, i, j, 4);
			sign = ((i+j)%2 == 0) ? 1 : -1;
			adjoint[i][j] = (sign)*(findDeterminant(temp, 3));
		}
	}
}

//find inverse of tf and return it
struct transform inverseTransform(struct transform tf) {
	double A[4][4];
	transferToMatrix(A, tf);
	double inverse[4][4];
	
	int det = findDeterminant(A, 4);
	if (det == 0) {
		logr(error, "No inverse for given transform!\n");
	}
	
	double adjoint[4][4];
	findAdjoint(A, adjoint);
	
	for (int i = 0; i < 4; i++) {
		for (int j = 0; j < 4; j++) {
			inverse[i][j] = adjoint[i][j] / det;
		}
	}
	
	struct transform inversetf = {0};
	transferFromMatrix(&inversetf, inverse);
	
	return inversetf;
}

//FIXME: Untested
struct transform transpose(struct transform tf) {
	struct transform inv = emptyTransform();
	inv.a = tf.a; inv.b = tf.e; inv.c = tf.i; inv.d = inv.m;
	inv.e = tf.b; inv.f = tf.f; inv.g = tf.j; inv.h = tf.n;
	inv.i = tf.c; inv.j = tf.g; inv.k = tf.k; inv.l = tf.l;
	inv.m = tf.d; inv.n = tf.h; inv.o = tf.l; inv.p = tf.p;
	return inv;
}
