//
//  scene.h
//  C-Ray
//
//  Created by Valtteri Koskivuori on 28/02/15.
//  Copyright (c) 2015 Valtteri Koskivuori. All rights reserved.
//

#ifndef __C_Ray__scene__
#define __C_Ray__scene__

#include "includes.h"
#include "errorhandler.h"
#include "sphere.h"
#include "poly.h"
#include "camera.h"
#include "color.h"
#include "light.h"

//material
typedef struct {
	char name[MATERIAL_NAME_SIZE];
	char textureFilename[OBJ_FILENAME_LENGTH];
	color ambient;
    color diffuse;
	color specular;
	double reflectivity;
	double refractivity;
	double transparency;
	double sharpness;
	double glossiness;
	double refractionIndex;
}material;

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

//World
typedef struct {
	color *ambientColor;
	lightSphere *lights;
	material *materials;
	sphere *spheres;
	camera *camera;
	
	poly *polys;
	
	int sphereAmount;
	int polygonAmount;
	int materialAmount;
	int lightAmount;
	int objCount;
}world;

int testBuild(world *scene, char *inputFileName);

//This takes an input file, tokenizes it and applies it to a world object.
int buildScene(world *scene, char *inputFileName);

int vertexBuild(world *scene);

int loadOBJ(world *scene, char *inputFileName);

#endif /* defined(__C_Ray__scene__) */
