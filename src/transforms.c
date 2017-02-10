//
//  transforms.c
//  C-Ray
//
//  Created by Valtteri Koskivuori on 07/02/2017.
//  Copyright © 2017 Valtteri Koskivuori. All rights reserved.
//

#include "transforms.h"

matrixTransform emptyTransform() {
	matrixTransform transform;
	transform.type = transformTypeNone;
	transform.a = 0;transform.b = 0;transform.c = 0;transform.d = 0;
	transform.e = 0;transform.f = 0;transform.g = 0;transform.h = 0;
	transform.i = 0;transform.j = 0;transform.k = 0;transform.l = 0;
	transform.m = 0;transform.n = 0;transform.o = 0;transform.p = 0;
	return transform;
}

//http://tinyurl.com/hte35pq
void transformVector(vector *vec, matrixTransform tf) {
	if (!vec->isTransformed) {
		vec->x = (tf.a * vec->x) + (tf.b * vec->y) + (tf.c * vec->z) + tf.d;
		vec->y = (tf.e * vec->x) + (tf.f * vec->y) + (tf.g * vec->z) + tf.h;
		vec->z = (tf.i * vec->x) + (tf.j * vec->y) + (tf.k * vec->z) + tf.l;
		vec->isTransformed = true;
	}
}

void transformMesh(crayOBJ *object) {
	for (int tf = 0; tf < object->transformCount; tf++) {
		//Perform transforms
		for (int p = object->firstPolyIndex; p < object->polyCount; p++) {
			for (int v = 0; v < polygonArray->vertexCount; v++) {
				transformVector(&vertexArray[polygonArray[p].vertexIndex[v]], object->transforms[tf]);
			}
		}
		//Clear isTransformed flags
		for (int p = object->firstPolyIndex; p < object->polyCount; p++) {
			for (int v = 0; v < polygonArray->vertexCount; v++) {
				vertexArray[polygonArray[p].vertexIndex[v]].isTransformed = false;
			}
		}
	}
}

camera transformCamera(camera camera, matrixTransform transform) {
	//TODO
	return camera;
}

sphere transformSphere(sphere inputSphere, matrixTransform transform) {
	//TODO
	return inputSphere;
}

light transformLight(light inputLight, matrixTransform tf) {
	//TODO
	return inputLight;
}

matrixTransform newTransformRotateX(float degrees) {
	matrixTransform transform = emptyTransform();
	transform.type = transformTypeXRotate;
	transform.a = 1;
	transform.f = cos(degrees);
	transform.g = -sin(degrees);
	transform.j = sin(degrees);
	transform.k = cos(degrees);
	transform.p = 1;
	return transform;
}

matrixTransform newTransformRotateY(float degrees) {
	matrixTransform transform = emptyTransform();
	transform.type = transformTypeYRotate;
	transform.a = cos(degrees);
	transform.c = sin(degrees);
	transform.f = 1;
	transform.i = -sin(degrees);
	transform.k = cos(degrees);
	transform.p = 1;
	return transform;
}

matrixTransform newTransformRotateZ(float degrees) {
	matrixTransform transform = emptyTransform();
	transform.type = transformTypeZRotate;
	transform.a = cos(degrees);
	transform.b = -sin(degrees);
	transform.e = sin(degrees);
	transform.f = cos(degrees);
	transform.k = 1;
	transform.p = 1;
	return transform;
}

matrixTransform newTransformTranslate(double x, double y, double z) {
	matrixTransform transform = emptyTransform();
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

matrixTransform newTransformScale(double x, double y, double z) {
	matrixTransform transform = emptyTransform();
	transform.type = transformTypeScale;
	transform.a = x;
	transform.f = y;
	transform.k = z;
	transform.p = 1;
	return transform;
}
