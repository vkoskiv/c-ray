//
//  scene.h
//  C-ray
//
//  Created by Valtteri Koskivuori on 28/02/2015.
//  Copyright © 2015-2019 Valtteri Koskivuori. All rights reserved.
//

#pragma once

struct crayOBJ;
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
	struct gradient *ambientColor;
	
	//OBJ 3D models
	struct crayOBJ *objs;
	int objCount;
	
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
	int bounces;
};

void loadScene(struct renderer *r, char *filename, bool fromStdin);

void freeScene(struct world *scene);
