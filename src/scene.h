//
//  scene.h
//  C-Ray
//
//  Created by Valtteri Koskivuori on 28/02/15.
//  Copyright (c) 2015-2018 Valtteri Koskivuori. All rights reserved.
//

#pragma once

struct crayOBJ;
struct light;
struct material;
struct sphere;
struct camera;
struct matrixTransform;
struct poly;
struct kdTreeNode;
struct renderer;
struct display;

//World
struct world {
	//Ambient background color.
	struct color *ambientColor;
	
	//OBJ 3D models
	struct crayOBJ *objs;
	int objCount;
	
	//Lights
	struct light *lights;
	int lightCount;
	
	//Materials
	struct material *materials;
	int materialCount;
	
	//Spheres
	struct sphere *spheres;
	int sphereCount;
	
	//Currently only one camera supported
	struct camera *camera;
	int cameraCount;
	
	//Scene preferences
	double contrast;
	bool areaLights;
	int bounces;
};

int testBuild(struct renderer *renderer, char *inputFileName);
int parseJSON(struct renderer *r, char *inputFileName);
