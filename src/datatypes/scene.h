//
//  scene.h
//  C-ray
//
//  Created by Valtteri Koskivuori on 28/02/2015.
//  Copyright © 2015-2019 Valtteri Koskivuori. All rights reserved.
//

#pragma once

struct mesh;
struct material;
struct sphere;
struct camera;
struct transform;
struct poly;
struct kdTreeNode;
struct renderer;
struct display;

//World
struct world {
	//Ambient background color.
	struct gradient ambientColor;
	
	//Optional environment map
	struct texture *hdr;
	
	//3D models
	struct mesh *meshes;
	int meshCount;
	
	//Spheres
	struct sphere *spheres;
	int sphereCount;
	
	//Currently only one camera supported
	struct camera *camera;
	int cameraCount;
};

int loadScene(struct renderer *r, int argc, char **argv);

void freeScene(struct world *scene);

char *getFileName(char *input);
