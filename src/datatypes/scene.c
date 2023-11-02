//
//  scene.c
//  C-ray
//
//  Created by Valtteri Koskivuori on 28/02/2015.
//  Copyright © 2015-2022 Valtteri Koskivuori. All rights reserved.
//

#include "../includes.h"
#include "scene.h"

#include "../utils/timer.h"
#include "../utils/loaders/sceneloader.h"
#include "../utils/logging.h"
#include "image/imagefile.h"
#include "../renderer/renderer.h"
#include "image/texture.h"
#include "camera.h"
#include "../accelerators/bvh.h"
#include "tile.h"
#include "mesh.h"
#include "poly.h"
#include "../utils/platform/thread.h"
#include "../utils/platform/signal.h"
#include "../renderer/instance.h"
#include "../datatypes/bbox.h"
#include "../utils/mempool.h"
#include "../utils/hashtable.h"
#include "../nodes/bsdfnode.h"
#include "../utils/args.h"
#include "../utils/textbuffer.h"

struct bvh_build_task {
	struct bvh *bvh;
	const struct mesh *mesh;
};

void *bvh_build_thread(void *arg) {
	block_signals();
	struct bvh_build_task *task = (struct bvh_build_task *)thread_user_data(arg);
	task->bvh = build_mesh_bvh(task->mesh);
	return NULL;
}

static void compute_accels(struct mesh *meshes, int mesh_count) {
	logr(info, "Computing BVHs: ");
	struct timeval timer = { 0 };
	timer_start(&timer);
	struct bvh_build_task *tasks = calloc(mesh_count, sizeof(*tasks));
	struct cr_thread *build_threads = calloc(mesh_count, sizeof(*build_threads));
	for (int t = 0; t < mesh_count; ++t) {
		tasks[t] = (struct bvh_build_task){
			.mesh = &meshes[t],
		};
		build_threads[t] = (struct cr_thread){
			.thread_fn = bvh_build_thread,
			.user_data = &tasks[t]
		};
		if (thread_start(&build_threads[t])) {
			logr(error, "Failed to create a bvhBuildTask\n");
		}
	}
	
	for (int t = 0; t < mesh_count; ++t) {
		thread_wait(&build_threads[t]);
		meshes[t].bvh = tasks[t].bvh;
	}
	printSmartTime(timer_get_ms(timer));
	free(tasks);
	free(build_threads);
	logr(plain, "\n");
}

struct bvh *computeTopLevelBvh(struct instance *instances, int instanceCount) {
	logr(info, "Computing top-level BVH: ");
	struct timeval timer = {0};
	timer_start(&timer);
	struct bvh *new = build_top_level_bvh(instances, instanceCount);
	printSmartTime(timer_get_ms(timer));
	logr(plain, "\n");
	return new;
}

static void printSceneStats(struct world *scene, unsigned long long ms) {
	logr(info, "Scene construction completed in ");
	printSmartTime(ms);
	uint64_t polys = 0;
	uint64_t vertices = 0;
	uint64_t normals = 0;
	for (int i = 0; i < scene->instanceCount; ++i) {
		if (isMesh(&scene->instances[i])) {
			const struct mesh *mesh = scene->instances[i].object;
			polys += mesh->poly_count;
			vertices += mesh->vertex_count;
			normals += mesh->normal_count;
		}
	}
	logr(plain, "\n");
	logr(info, "Totals: %liV, %liN, %iI, %liP, %iS, %iM\n",
		   vertices,
		   normals,
		   scene->instanceCount,
		   polys,
		   scene->sphereCount,
		   scene->meshCount);
}

//Split scene loading and prefs?
//Load the scene, allocate buffers, etc
//FIXME: Rename this func and take parseJSON out to a separate call.
int loadScene(struct renderer *r, char *input) {
	
	struct timeval timer = {0};
	timer_start(&timer);
	
	cJSON *json = cJSON_Parse(input);
	if (!json) {
		const char *errptr = cJSON_GetErrorPtr();
		if (errptr) {
			logr(warning, "Failed to parse JSON\n");
			logr(warning, "Error before: %s\n", errptr);
			return -2;
		}
	}
	// Input is potentially very large, so we free it as soon as possible
	// FIXME: mmap() input
	free(input);

	//Load configuration and assets
	//FIXME: Rename parseJSON
	//FIXME: Actually throw out all of this code in this function and rewrite
	switch (parseJSON(r, json)) {
		case -1:
			cJSON_Delete(json);
			logr(warning, "Scene builder failed due to previous error.\n");
			return -1;
		case 4:
			cJSON_Delete(json);
			logr(warning, "Scene debug mode enabled, won't render image.\n");
			return -1;
		case -2:
			cJSON_Delete(json);
			//JSON parser failed
			return -1;
		default:
			break;
	}
	
	// This is where we prepare a cache of scene data to be sent to worker nodes
	// We also apply any potential command-line overrides to that cache here as well.
	// FIXME: This overrides setting should be integrated with scene loading, probably.
	if (isSet("use_clustering")) {
		// Stash a cache of scene data here
		// Apply overrides to the cache here
		if (isSet("samples_override")) {
			cJSON *renderer = cJSON_GetObjectItem(json, "renderer");
			if (cJSON_IsObject(renderer)) {
				int samples = intPref("samples_override");
				logr(debug, "Overriding cache sample count to %i\n", samples);
				if (cJSON_IsNumber(cJSON_GetObjectItem(renderer, "samples"))) {
					cJSON_ReplaceItemInObject(renderer, "samples", cJSON_CreateNumber(samples));
				} else {
					cJSON_AddItemToObject(renderer, "samples", cJSON_CreateNumber(samples));
				}
			}
		}
		
		if (isSet("dims_override")) {
			cJSON *renderer = cJSON_GetObjectItem(json, "renderer");
			if (cJSON_IsObject(renderer)) {
				int width = intPref("dims_width");
				int height = intPref("dims_height");
				logr(info, "Overriding cache image dimensions to %ix%i\n", width, height);
				if (cJSON_IsNumber(cJSON_GetObjectItem(renderer, "width")) && cJSON_IsNumber(cJSON_GetObjectItem(renderer, "height"))) {
					cJSON_ReplaceItemInObject(renderer, "width", cJSON_CreateNumber(width));
					cJSON_ReplaceItemInObject(renderer, "height", cJSON_CreateNumber(height));
				} else {
					cJSON_AddItemToObject(renderer, "width", cJSON_CreateNumber(width));
					cJSON_AddItemToObject(renderer, "height", cJSON_CreateNumber(height));
				}
			}
		}
		
		if (isSet("tiledims_override")) {
			cJSON *renderer = cJSON_GetObjectItem(json, "renderer");
			if (cJSON_IsObject(renderer)) {
				int width = intPref("tile_width");
				int height = intPref("tile_height");
				logr(info, "Overriding cache tile dimensions to %ix%i\n", width, height);
				if (cJSON_IsNumber(cJSON_GetObjectItem(renderer, "tileWidth")) && cJSON_IsNumber(cJSON_GetObjectItem(renderer, "tileHeight"))) {
					cJSON_ReplaceItemInObject(renderer, "tileWidth", cJSON_CreateNumber(width));
					cJSON_ReplaceItemInObject(renderer, "tileHeight", cJSON_CreateNumber(height));
				} else {
					cJSON_AddItemToObject(renderer, "tileWidth", cJSON_CreateNumber(width));
					cJSON_AddItemToObject(renderer, "tileHeight", cJSON_CreateNumber(height));
				}
			}
		}

		if (r->prefs.selected_camera != 0) {
			cJSON_AddItemToObject(json, "selected_camera", cJSON_CreateNumber(r->prefs.selected_camera));
		}

		// Store cache. This is what gets sent to worker nodes.
		r->sceneCache = cJSON_PrintUnformatted(json);
	}
	
	logr(debug, "Deleting JSON...\n");
	cJSON_Delete(json);
	logr(debug, "Deleting done\n");

	if (r->prefs.threadCount > 0) {
		// Do some pre-render preparations
		// Compute BVH acceleration structures for all objects in the scene
		compute_accels(r->scene->meshes, r->scene->meshCount);
		// And then compute a single top-level BVH that contains all the objects
		r->scene->topLevel = computeTopLevelBvh(r->scene->instances, r->scene->instanceCount);
		printSceneStats(r->scene, timer_get_ms(timer));
	} else {
		logr(debug, "No local render threads, skipping local BVH construction.\n");
	}
	
	//Quantize image into renderTiles
	r->state.tileCount = quantizeImage(&r->state.renderTiles,
									   r->scene->cameras[r->prefs.selected_camera].width,
									   r->scene->cameras[r->prefs.selected_camera].height,
									   r->prefs.tileWidth,
									   r->prefs.tileHeight,
									   r->prefs.tileOrder);
	
	// Some of this stuff seems like it should be in newRenderer(), but notice
	// how they depend on r->prefs, which is populated by parseJSON

	struct camera cam = r->scene->cameras[r->prefs.selected_camera];
	//Allocate memory for render buffer
	//Render buffer is used to store accurate color values for the renderers' internal use
	r->state.renderBuffer = newTexture(float_p, cam.width, cam.height, 3);
	
	//Allocate memory for render UI buffer
	//This buffer is used for storing UI stuff like currently rendering tile highlights
	r->state.uiBuffer = newTexture(char_p, cam.width, cam.height, 4);
	
	//Print a useful warning to user if the defined tile size results in less renderThreads
	if (r->state.tileCount < r->prefs.threadCount) {
		logr(warning, "WARNING: Rendering with a less than optimal thread count due to large tile size!\n");
		logr(warning, "Reducing thread count from %i to %i\n", r->prefs.threadCount, r->state.tileCount);
		r->prefs.threadCount = r->state.tileCount;
	}
	return 0;
}

//Free scene data
void destroyScene(struct world *scene) {
	if (scene) {
		free(scene->cameras);
		for (int i = 0; i < scene->meshCount; ++i) {
			destroyMesh(&scene->meshes[i]);
		}
		destroy_bvh(scene->topLevel);
		destroyHashtable(scene->storage.node_table);
		destroyBlocks(scene->storage.node_pool);
		for (int i = 0; i < scene->instanceCount; ++i) {
			if (scene->instances[i].bsdfs) free(scene->instances[i].bsdfs);
		}
		free(scene->instances);
		free(scene->meshes);
		free(scene->spheres);
		free(scene);
	}
}
