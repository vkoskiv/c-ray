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
#include "../utils/dyn_array.h"

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

static void compute_accels(struct mesh_arr meshes) {
	logr(info, "Computing BVHs: ");
	struct timeval timer = { 0 };
	timer_start(&timer);
	struct bvh_build_task *tasks = calloc(meshes.count, sizeof(*tasks));
	struct cr_thread *build_threads = calloc(meshes.count, sizeof(*build_threads));
	for (size_t t = 0; t < meshes.count; ++t) {
		tasks[t] = (struct bvh_build_task){
			.mesh = &meshes.items[t],
		};
		build_threads[t] = (struct cr_thread){
			.thread_fn = bvh_build_thread,
			.user_data = &tasks[t]
		};
		if (thread_start(&build_threads[t])) {
			logr(error, "Failed to create a bvhBuildTask\n");
		}
	}
	
	for (size_t t = 0; t < meshes.count; ++t) {
		thread_wait(&build_threads[t]);
		meshes.items[t].bvh = tasks[t].bvh;
	}
	printSmartTime(timer_get_ms(timer));
	free(tasks);
	free(build_threads);
	logr(plain, "\n");
}

struct bvh *computeTopLevelBvh(struct instance_arr instances) {
	logr(info, "Computing top-level BVH: ");
	struct timeval timer = {0};
	timer_start(&timer);
	struct bvh *new = build_top_level_bvh(instances);
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
	for (size_t i = 0; i < scene->instances.count; ++i) {
		if (isMesh(&scene->instances.items[i])) {
			const struct mesh *mesh = &scene->meshes.items[scene->instances.items[i].object_idx];
			polys += mesh->polygons.count;
			vertices += mesh->vbuf->vertices.count;
			normals += mesh->vbuf->normals.count;
		}
	}
	logr(plain, "\n");
	logr(info, "Totals: %liV, %liN, %zuI, %liP, %zuS, %zuM\n",
		   vertices,
		   normals,
		   scene->instances.count,
		   polys,
		   scene->spheres.count,
		   scene->meshes.count);
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

	if (r->prefs.threads > 0) {
		// Do some pre-render preparations
		// Compute BVH acceleration structures for all objects in the scene
		compute_accels(r->scene->meshes);
		// And then compute a single top-level BVH that contains all the objects
		r->scene->topLevel = computeTopLevelBvh(r->scene->instances);
		printSceneStats(r->scene, timer_get_ms(timer));
	} else {
		logr(debug, "No local render threads, skipping local BVH construction.\n");
	}
	
	//Quantize image into renderTiles
	tile_quantize(&r->state.tiles,
					r->scene->cameras.items[r->prefs.selected_camera].width,
					r->scene->cameras.items[r->prefs.selected_camera].height,
					r->prefs.tileWidth,
					r->prefs.tileHeight,
					r->prefs.tileOrder);

	for (size_t i = 0; i < r->state.tiles.count; ++i)
		r->state.tiles.items[i].total_samples = r->prefs.sampleCount;
	
	//Print a useful warning to user if the defined tile size results in less renderThreads
	if (r->state.tiles.count < r->prefs.threads) {
		logr(warning, "WARNING: Rendering with a less than optimal thread count due to large tile size!\n");
		logr(warning, "Reducing thread count from %zu to %zu\n", r->prefs.threads, r->state.tiles.count);
		r->prefs.threads = r->state.tiles.count;
	}
	return 0;
}

//Free scene data
void destroyScene(struct world *scene) {
	if (scene) {
		camera_arr_free(&scene->cameras);
		for (size_t i = 0; i < scene->meshes.count; ++i) {
			destroyMesh(&scene->meshes.items[i]);
		}
		mesh_arr_free(&scene->meshes);
		destroy_bvh(scene->topLevel);
		destroyHashtable(scene->storage.node_table);
		destroyBlocks(scene->storage.node_pool);
		for (size_t i = 0; i < scene->instances.count; ++i) {
			bsdf_buf_unref(scene->instances.items[i].bbuf);
		}
		instance_arr_free(&scene->instances);
		sphere_arr_free(&scene->spheres);
		free(scene);
	}
}
