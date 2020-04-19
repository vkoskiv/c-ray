//
//  scene.c
//  C-ray
//
//  Created by Valtteri Koskivuori on 28/02/2015.
//  Copyright © 2015-2020 Valtteri Koskivuori. All rights reserved.
//

#include "../includes.h"
#include "scene.h"

#include "../utils/timer.h"
#include "../utils/loaders/sceneloader.h"
#include "../utils/logging.h"
#include "image/imagefile.h"
#include "../renderer/renderer.h"
#include "image/texture.h"
#include "image/hdr.h"
#include "camera.h"
#include "vertexbuffer.h"
#include "../accelerators/kdtree.h"
#include "tile.h"
#include "mesh.h"
#include "poly.h"
#include "../utils/platform/thread.h"
#include "../utils/args.h"
#include "../utils/ui.h"

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
		for (int j = 0; j < meshes[i].polyCount; ++j) {
			indices[j] = meshes[i].firstPolyIndex + j;
		}
		meshes[i].tree = buildTree(indices, meshes[i].polyCount);
		
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

void checkAndSetCliOverrides(struct renderer *r) {
	//Update threadCount if it's overridden
	if (isSet("thread_override")) {
		int threads = intPref("thread_override");
		logr(info, "Overriding thread count to %i\n", threads);
		r->prefs.threadCount = threads;
		r->prefs.fromSystem = false;
	}
	
	//Update image dimensions if it's overridden
	if (isSet("dims_override")) {
		int width = intPref("dims_width");
		int height = intPref("dims_height");
		logr(info, "Overriding image dimensions to %ix%i\n", width, height);
		r->prefs.imageWidth = intPref("dims_width");
		r->prefs.imageHeight = intPref("dims_height");
		r->mainDisplay->width = r->prefs.imageWidth;
		r->mainDisplay->height = r->prefs.imageHeight;
	}
}

//Split scene loading and prefs?
//Load the scene, allocate buffers, etc
//FIXME: Rename this func and take parseJSON out to a separate call.
int loadScene(struct renderer *r, char *input) {
	
	struct timeval timer = {0};
	startTimer(&timer);
	
	//Build the scene
	switch (parseJSON(r, input)) {
		case -1:
			logr(warning, "Scene builder failed due to previous error.\n");
			return -1;
		case 4:
			logr(warning, "Scene debug mode enabled, won't render image.\n");
			return -1;
		case -2:
			//JSON parser failed
			return -1;
		default:
			break;
	}
	
	//FIXME: Temporary. Just make the ui module configure itself.
	r->mainDisplay->width = r->prefs.imageWidth;
	r->mainDisplay->height = r->prefs.imageHeight;
	
	checkAndSetCliOverrides(r);
	
	transformCameraIntoView(r->scene->camera);
	transformMeshes(r->scene);
	computeKDTrees(r->scene->meshes, r->scene->meshCount);
	printSceneStats(r->scene, getMs(timer));
	
	//Quantize image into renderTiles
	r->state.tileCount = quantizeImage(&r->state.renderTiles,
									   r->prefs.imageWidth,
									   r->prefs.imageHeight,
									   r->prefs.tileWidth,
									   r->prefs.tileHeight,
									   r->prefs.tileOrder);
	
	//Compute the focal length for the camera
	computeFocalLength(r->scene->camera, r->prefs.imageWidth);
	
	//Allocate memory for render buffer
	//Render buffer is used to store accurate color values for the renderers' internal use
	r->state.renderBuffer = newTexture(float_p, r->prefs.imageWidth, r->prefs.imageHeight, 3);
	
	//Allocate memory for render UI buffer
	//This buffer is used for storing UI stuff like currently rendering tile highlights
	r->state.uiBuffer = newTexture(char_p, r->prefs.imageWidth, r->prefs.imageHeight, 4);
	
	//Some of this stuff seems like it should be in newRenderer(), but notice
	//how it depends on r->prefs, which is populated by parseJSON
	//Alloc memory for crThreads
	r->state.threads = calloc(r->prefs.threadCount, sizeof(struct crThread));
	if (r->state.threads == NULL) {
		logr(error, "Failed to allocate memory for crThreads.\n");
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
void destroyScene(struct world *scene) {
	if (scene) {
		destroyHDRI(scene->hdr);
		if (scene->meshes) {
			for (int i = 0; i < scene->meshCount; ++i) {
				destroyMesh(&scene->meshes[i]);
			}
			free(scene->meshes);
		}
		if (scene->spheres) {
			free(scene->spheres);
		}
		if (scene->camera) {
			destroyCamera(scene->camera);
		}
		free(scene);
	}
}
