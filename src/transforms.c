//
//  transforms.c
//  C-Ray
//
//  Created by Valtteri Koskivuori on 07/02/2017.
//  Copyright © 2017 Valtteri Koskivuori. All rights reserved.
//

#include "transforms.h"

//For ease of use
double toRadians(double degrees) {
	return (degrees * PI) / 180;
}

void addTransform(crayOBJ *obj, matrixTransform transform) {
	if (obj->transformCount == 0) {
		obj->transforms = (matrixTransform*)calloc(1, sizeof(matrixTransform));
	} else {
		obj->transforms = (matrixTransform*)realloc(obj->transforms, (obj->transformCount + 1) * sizeof(matrixTransform));
	}
	
	obj->transforms[obj->transformCount] = transform;
	obj->transformCount++;
}

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
void transformVector(vector *vec, matrixTransform *tf) {
	if (!vec->isTransformed) {
		vector temp;
		temp.x = (tf->a * vec->x) + (tf->b * vec->y) + (tf->c * vec->z) + tf->d;
		temp.y = (tf->e * vec->x) + (tf->f * vec->y) + (tf->g * vec->z) + tf->h;
		temp.z = (tf->i * vec->x) + (tf->j * vec->y) + (tf->k * vec->z) + tf->l;
		vec->x = temp.x;
		vec->y = temp.y;
		vec->z = temp.z;
		vec->isTransformed = true;
	}
}

void transformMesh(crayOBJ *object) {
	for (int tf = 0; tf < object->transformCount; tf++) {
		//Perform transforms
		for (int p = object->firstPolyIndex; p < (object->firstPolyIndex + object->polyCount); p++) {
			for (int v = 0; v < polygonArray[p].vertexCount; v++) {
				transformVector(&vertexArray[polygonArray[p].vertexIndex[v]], &object->transforms[tf]);
			}
		}
		//Clear isTransformed flags
		for (int p = object->firstPolyIndex; p < object->firstPolyIndex + object->polyCount; p++) {
			for (int v = 0; v < polygonArray->vertexCount; v++) {
				vertexArray[polygonArray[p].vertexIndex[v]].isTransformed = false;
			}
		}
	}
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
	transform.f = cos(toRadians(degrees));
	transform.g = -sin(toRadians(degrees));
	transform.j = sin(toRadians(degrees));
	transform.k = cos(toRadians(degrees));
	transform.p = 1;
	return transform;
}

matrixTransform newTransformRotateY(float degrees) {
	matrixTransform transform = emptyTransform();
	transform.type = transformTypeYRotate;
	transform.a = cos(toRadians(degrees));
	transform.c = sin(toRadians(degrees));
	transform.f = 1;
	transform.i = -sin(toRadians(degrees));
	transform.k = cos(toRadians(degrees));
	transform.p = 1;
	return transform;
}

matrixTransform newTransformRotateZ(float degrees) {
	matrixTransform transform = emptyTransform();
	transform.type = transformTypeZRotate;
	transform.a = cos(toRadians(degrees));
	transform.b = -sin(toRadians(degrees));
	transform.e = sin(toRadians(degrees));
	transform.f = cos(toRadians(degrees));
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
