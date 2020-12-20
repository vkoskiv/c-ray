//
//  scene.h
//  C-ray
//
//  Created by Valtteri Koskivuori on 28/02/2015.
//  Copyright © 2015-2020 Valtteri Koskivuori. All rights reserved.
//

#pragma once

struct renderer;
struct hashtable;

struct world {
	//Optional environment map / ambient color
	const struct bsdfNode *background;
	
	struct mesh *meshes;
	int meshCount;
	
	struct instance *instances;
	int instanceCount;
	
	// Top-level bounding volume hierarchy,
	// contains all 3D assets in the scene.
	struct bvh *topLevel;
	
	struct sphere *spheres;
	int sphereCount;
	
	//Currently only one camera supported
	struct camera *camera;
	int cameraCount;
	
	// Scene asset memory pool, currently used for nodes only.
	struct block *nodePool;
	// Used for hash consing. (preventing duplicate nodes)
	struct hashtable *nodeTable;
};

int loadScene(struct renderer *r, char *input);

void destroyScene(struct world *scene);
