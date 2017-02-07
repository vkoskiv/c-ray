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
	transformTypeMultiplication
}transformType;

//Reference: http://tinyurl.com/ho6h6mr
typedef struct {
	transformType type;
	int a, b, c, d;
	int e, f, g, h;
	int i, j, k, l;
	int m, n, o, p;
}matrixTransform;

typedef struct {
	material material;
	poly *polygons;
	matrixTransform *transforms;
}crayOBJ;

#endif /* transforms_h */
