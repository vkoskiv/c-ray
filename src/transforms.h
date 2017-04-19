//
//  transforms.h
//  C-Ray
//
//  Created by Valtteri Koskivuori on 07/02/2017.
//  Copyright © 2017 Valtteri Koskivuori. All rights reserved.
//

#ifndef transforms_h
#define transforms_h

#include "includes.h"
#include "vector.h"
#include "color.h"
#include "poly.h"
#include "camera.h"
#include "sphere.h"
#include "light.h"

typedef enum {
	transformTypeXRotate,
	transformTypeYRotate,
	transformTypeZRotate,
	transformTypeTranslate,
	transformTypeScale,
	transformTypeMultiplication,
	transformTypeNone
}transformType;

//Reference: http://tinyurl.com/ho6h6mr
typedef struct {
	transformType type;
	double a, b, c, d;
	double e, f, g, h;
	double i, j, k, l;
	double m, n, o, p;
}matrixTransform;

typedef struct {
	int vertexCount;
	int firstVectorIndex;
	
	int normalCount;
	int firstNormalIndex;
	
	int textureCount;
	int firstTextureIndex;
	
	int polyCount;
	int firstPolyIndex;
	
	sphere boundingVolume;
	matrixTransform *transforms;
	int transformCount;
}crayOBJ;

//Transform types
matrixTransform newTransformScale(double x, double y, double z);
matrixTransform newTransformTranslate(double x, double y, double z);
matrixTransform newTransformRotateX(float degrees);
matrixTransform newTransformRotateY(float degrees);
matrixTransform newTransformRotateZ(float degrees);
matrixTransform emptyTransform();

//Object transforms
void transformMesh(crayOBJ *object);

//Scene builder
void addTransform(crayOBJ *obj, matrixTransform transform);

#endif /* transforms_h */
