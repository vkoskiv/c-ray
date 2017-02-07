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
	transform.a = 0;transform.b = 0;transform.c = 0;transform.d = 0;
	transform.e = 0;transform.f = 0;transform.g = 0;transform.h = 0;
	transform.i = 0;transform.j = 0;transform.k = 0;transform.l = 0;
	transform.m = 0;transform.n = 0;transform.o = 0;transform.p = 0;
	return transform;
}

vector trasformVector(vector vec, matrixTransform tf) {
	vector transformed;
	transformed.x = tf.a * vec.x + tf.b * vec.y + tf.c * vec.z + tf.d;
	transformed.y = tf.e * vec.x + tf.f * vec.y + tf.g * vec.z + tf.h;
	transformed.z = tf.i * vec.x + tf.j * vec.y + tf.k * vec.z + tf.l;
	return transformed;
}

crayOBJ transformMesh(crayOBJ object, matrixTransform transform) {
	//TODO
	return object;
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
	transform.a = x;
	transform.f = y;
	transform.k = z;
	transform.p = 1;
	return transform;
}
