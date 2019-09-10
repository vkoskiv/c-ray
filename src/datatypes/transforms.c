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

//FIXME: Return and rename to identity matrix
struct transform emptyTransform() {
	struct transform transform;
	transform.type = transformTypeNone;
	for (int i = 0; i < 4; i++) {
		for (int j = 0; j < 4; j++) {
			transform.A[i][j] = 0;
		}
	}
	return transform;
}

struct transform fromParams(double t00, double t01, double t02, double t03,
							double t10, double t11, double t12, double t13,
							double t20, double t21, double t22, double t23,
							double t30, double t31, double t32, double t33) {
	struct transform new = emptyTransform();
	new.type = transformTypeNone;
	new.A[0][0] = t00; new.A[0][1] = t01; new.A[0][2] = t02; new.A[0][3] = t03;
	new.A[1][0] = t10; new.A[1][1] = t11; new.A[1][2] = t12; new.A[1][3] = t13;
	new.A[2][0] = t20; new.A[2][1] = t21; new.A[2][2] = t22; new.A[2][3] = t23;
	new.A[3][0] = t30; new.A[3][1] = t31; new.A[3][2] = t32; new.A[3][3] = t33;
	return new;
}

//http://tinyurl.com/hte35pq
void transformVector(struct vector *vec, struct transform *tf) {
	struct vector temp;
	temp.x = (tf->A[0][0] * vec->x) + (tf->A[0][1] * vec->y) + (tf->A[0][2] * vec->z) + tf->A[0][3];
	temp.y = (tf->A[1][0] * vec->x) + (tf->A[1][1] * vec->y) + (tf->A[1][2] * vec->z) + tf->A[1][3];
	temp.z = (tf->A[2][0] * vec->x) + (tf->A[2][1] * vec->y) + (tf->A[2][2] * vec->z) + tf->A[2][3];
	vec->x = temp.x;
	vec->y = temp.y;
	vec->z = temp.z;
}

struct transform newTransformRotateX(double degrees) {
	struct transform transform = emptyTransform();
	double rads = toRadians(degrees);
	transform.type = transformTypeXRotate;
	transform.A[0][0] = 1;
	transform.A[1][1] = cos(rads);
	transform.A[1][2] = -sin(rads);
	transform.A[2][1] = sin(rads);
	transform.A[2][2] = cos(rads);
	transform.A[3][3] = 1;
	return transform;
}

struct transform newTransformRotateY(double degrees) {
	struct transform transform = emptyTransform();
	double rads = toRadians(degrees);
	transform.type = transformTypeYRotate;
	transform.A[0][0] = cos(rads);
	transform.A[0][2] = sin(rads);
	transform.A[1][1] = 1;
	transform.A[2][0] = -sin(rads);
	transform.A[2][2] = cos(rads);
	transform.A[3][3] = 1;
	return transform;
}

struct transform newTransformRotateZ(double degrees) {
	struct transform transform = emptyTransform();
	double rads = toRadians(degrees);
	transform.type = transformTypeZRotate;
	transform.A[0][0] = cos(rads);
	transform.A[0][1] = -sin(rads);
	transform.A[1][0] = sin(rads);
	transform.A[1][1] = cos(rads);
	transform.A[2][2] = 1;
	transform.A[3][3] = 1;
	return transform;
}

struct transform newTransformTranslate(double x, double y, double z) {
	struct transform transform = emptyTransform();
	transform.type = transformTypeTranslate;
	transform.A[0][0] = 1;
	transform.A[1][1] = 1;
	transform.A[2][2] = 1;
	transform.A[3][3] = 1;
	transform.A[0][3] = x;
	transform.A[1][3] = y;
	transform.A[2][3] = z;
	return transform;
}

struct transform newTransformScale(double x, double y, double z) {
	struct transform transform = emptyTransform();
	transform.type = transformTypeScale;
	transform.A[0][0] = x;
	transform.A[1][1] = y;
	transform.A[2][2] = z;
	transform.A[3][3] = 1;
	return transform;
}

struct transform newTransformScaleUniform(double scale) {
	struct transform transform = emptyTransform();
	transform.type = transformTypeScale;
	transform.A[0][0] = scale;
	transform.A[1][1] = scale;
	transform.A[2][2] = scale;
	transform.A[3][3] = 1;
	return transform;
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

struct transform inverse(struct transform tf) {
	struct transform inverse = {0};
	
	int det = findDeterminant(tf.A, 4);
	if (det == 0) {
		logr(error, "No inverse for given transform!\n");
	}
	
	double adjoint[4][4];
	findAdjoint(tf.A, adjoint);
	
	for (int i = 0; i < 4; i++) {
		for (int j = 0; j < 4; j++) {
			inverse.A[i][j] = adjoint[i][j] / det;
		}
	}
	
	inverse.type = transformTypeInverse;
	
	return inverse;
}

struct transform transpose(struct transform tf) {
	return fromParams(tf.A[0][0], tf.A[1][0], tf.A[2][0], tf.A[3][0],
					  tf.A[0][1], tf.A[1][1], tf.A[2][1], tf.A[3][1],
					  tf.A[0][2], tf.A[1][2], tf.A[2][2], tf.A[3][2],
					  tf.A[0][3], tf.A[1][3], tf.A[2][3], tf.A[3][3]);
}

//Tests, this is dead code for now until I get a testing suite.
void printMatrix(struct transform tf) {
	printf("\n");
	for (int i = 0; i < 4; i++) {
		for (int j = 0; j < 4; j++) {
			printf("tf.A[%i][%i]=%.2f ",i,j,tf.A[i][j]);
		}
		printf("\n");
	}
}

//Print the given matrix, and the inverse.
void testInverse(struct transform tf) {
	printf("Testing inverse...\n");
	printMatrix(tf);
	printMatrix(inverse(tf));
}

void testTranspose(struct transform tf) {
	printf("Testing transpose...\n");
	printMatrix(tf);
	printMatrix(transpose(tf));
}

void testInverseAndTranspose() {
	testInverse(fromParams(1, 0, 0, 1,
						   0, 2, 1, 2,
						   2, 1, 0, 1,
						   2, 0, 1, 4));
	
	testTranspose(fromParams(1, 0, 0, 1,
							 0, 2, 1, 2,
							 2, 1, 0, 1,
							 2, 0, 1, 4));
}
