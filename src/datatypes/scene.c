//
//  scene.c
//  C-ray
//
//  Created by Valtteri Koskivuori on 28/02/2015.
//  Copyright © 2015-2020 Valtteri Koskivuori. All rights reserved.
//

#include "../includes.h"
#include "scene.h"

#include "../utils/filehandler.h"
#include "../utils/timer.h"
#include "../utils/loaders/sceneloader.h"
#include "../utils/logging.h"
#include "../renderer/renderer.h"
#include "../datatypes/texture.h"
#include "../datatypes/camera.h"
#include "../datatypes/vertexbuffer.h"
#include "../acceleration/kdtree.h"
#include "../datatypes/tile.h"

void transformMeshes(struct world *scene) {
	logr(info, "Running transforms: ");
	struct timeval timer = {0};
	startTimer(&timer);
	for (int i = 0; i < scene->meshCount; ++i) {
		transformMesh(&scene->meshes[i]);
	}
	printSmartTime(getMs(timer));
	printf("\n");
}

//TODO: Parallelize this task
void computeKDTrees(struct mesh *meshes, int meshCount) {
	logr(info, "Computing KD-trees: ");
	struct timeval timer = {0};
	startTimer(&timer);
	for (int i = 0; i < meshCount; ++i) {
		int *indices = calloc(meshes[i].polyCount, sizeof(int));
		for (int j = 0; j < meshes[i].polyCount; j++) {
			indices[j] = meshes[i].firstPolyIndex + j;
		}
		meshes[i].tree = buildTree(indices, meshes[i].polyCount, 0);
		
		// Optional tree checking
		/*int orphans = checkTree(meshes[i].tree);
		if (orphans > 0) {
			int total = countNodes(meshes[i].tree);
			logr(warning, "Found %i/%i orphan nodes in %s kdtree\n", orphans, total, meshes[i].name);
		}*/
	}
	printSmartTime(getMs(timer));
	printf("\n");
}

void printSceneStats(struct world *scene, unsigned long long ms) {
	logr(info, "Scene construction completed in ");
	printSmartTime(ms);
	printf("\n");
	logr(info, "Totals: %iV, %iN, %iT, %iP, %iS\n",
		   vertexCount,
		   normalCount,
		   textureCount,
		   polyCount,
		   scene->sphereCount);
}

//Load the scene, allocate buffers, etc
//FIXME: Rename this func and take parseJSON out to a separate call.
int loadScene(struct renderer *r, int argc, char **argv) {
	
	bool fromStdin = false;
	char *input;
	if (argc == 2) {
		input = argv[1];
		fromStdin = false;
	} else {
		input = readStdin();
		fromStdin = true;
	}
	
	struct timeval timer = {0};
	startTimer(&timer);
	
	//Build the scene
	switch (parseJSON(r, input, fromStdin)) {
		case -1:
			logr(warning, "Scene builder failed due to previous error.\n");
			return -1;
			break;
		case 4:
			logr(warning, "Scene debug mode enabled, won't render image.\n");
			return -1;
			break;
		case -2:
			logr(warning, "JSON parser failed.\n");
			return -1;
			break;
		default:
			break;
	}
	
	transformCameraIntoView(r->scene->camera);
	transformMeshes(r->scene);
	computeKDTrees(r->scene->meshes, r->scene->meshCount);
	printSceneStats(r->scene, getMs(timer));
	
	//Quantize image into renderTiles
	r->state.tileCount = quantizeImage(&r->state.renderTiles, r->prefs.imageWidth, r->prefs.imageHeight, r->prefs.tileWidth, r->prefs.tileHeight);
	reorderTiles(&r->state.renderTiles, r->state.tileCount, r->prefs.tileOrder);
	
	//Compute the focal length for the camera
	computeFocalLength(r->scene->camera, r->prefs.imageWidth);
	
	//Allocate memory for render buffer
	//Render buffer is used to store accurate color values for the renderers' internal use
	r->state.renderBuffer = newTexture();
	allocTextureBuffer(r->state.renderBuffer, float_p, r->prefs.imageWidth, r->prefs.imageHeight, 3);
	
	//Allocate memory for render UI buffer
	//This buffer is used for storing UI stuff like currently rendering tile highlights
	r->state.uiBuffer = newTexture();
	allocTextureBuffer(r->state.uiBuffer, char_p, r->prefs.imageWidth, r->prefs.imageHeight, 4);
	
	//Alloc memory for pthread_create() args
	r->state.threadStates = calloc(r->prefs.threadCount, sizeof(struct threadState));
	if (r->state.threadStates == NULL) {
		logr(error, "Failed to allocate memory for threadInfo args.\n");
	}
	
	//Print a useful warning to user if the defined tile size results in less renderThreads
	if (r->state.tileCount < r->prefs.threadCount) {
		logr(warning, "WARNING: Rendering with a less than optimal thread count due to large tile size!\n");
		logr(warning, "Reducing thread count from %i to ", r->prefs.threadCount);
		r->prefs.threadCount = r->state.tileCount;
		printf("%i\n", r->prefs.threadCount);
	}
	return 0;
}

//Free scene data
void freeScene(struct world *scene) {
	if (scene->hdr) {
		freeTexture(scene->hdr);
		free(scene->hdr);
	}
	
	if (scene->meshes) {
		for (int i = 0; i < scene->meshCount; i++) {
			freeMesh(&scene->meshes[i]);
		}
		free(scene->meshes);
	}
	if (scene->spheres) {
		free(scene->spheres);
	}
	if (scene->camera) {
		freeCamera(scene->camera);
		free(scene->camera);
	}
}
