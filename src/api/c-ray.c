//
//  c-ray.c
//  C-ray
//
//  Created by Valtteri on 5.1.2020.
//  Copyright © 2020-2022 Valtteri Koskivuori. All rights reserved.
//

#include "../includes.h"
#include <c-ray/c-ray.h>

#include <stdarg.h>
#include <stdio.h>
#include <stdbool.h>

#include "../datatypes/image/imagefile.h"
#include "../renderer/renderer.h"
#include "../datatypes/scene.h"
#include "../utils/gitsha1.h"
#include "../utils/logging.h"
#include "../utils/fileio.h"
#include "../utils/platform/terminal.h"
#include "../utils/assert.h"
#include "../datatypes/image/texture.h"
#include "../utils/timer.h"
#include "../utils/args.h"
#include "../utils/encoders/encoder.h"
#include "../utils/string.h"
#include "../utils/protocol/server.h"
#include "../utils/protocol/worker.h"
#include "../utils/filecache.h"
#include "../utils/hashtable.h"
#include "../datatypes/camera.h"

#ifdef CRAY_DEBUG_ENABLED
#define DEBUG "D"
#else
#define DEBUG ""
#endif

#define VERSION "0.6.3"DEBUG

struct texture *currentImage = NULL;

char *cr_get_version() {
	return VERSION;
}

char *cr_get_git_hash() {
	return gitHash();
}

void cr_parse_args(int argc, char **argv) {
	parseArgs(argc, argv);
}

int cr_is_option_set(char *key) {
	return isSet(key);
}

char *cr_path_arg() {
	return pathArg();
}

void cr_destroy_options() {
	destroyOptions();
}

char *cr_get_file_path(char *full_path) {
	return get_file_path(full_path);
}

// -- Renderer --

struct cr_renderer;

struct cr_renderer *cr_new_renderer() {
	struct renderer *r = newRenderer();
	cr_set_asset_path(r);
	return r;
}

enum cr_renderer_param {
	cr_renderer_threads = 0,
	cr_renderer_samples,
	cr_renderer_bounces,
	cr_renderer_tile_width,
	cr_renderer_tile_height,
};

bool cr_renderer_set_num_pref(struct cr_renderer *ext, enum cr_renderer_param p, uint64_t num) {
	if (!ext) return false;
	struct renderer *r = (struct renderer *)ext;
	switch (p) {
	case cr_renderer_threads: {
		r->prefs.threads = num;
		return true;
	}
	case cr_renderer_samples: {
		r->prefs.sampleCount = num;
		return true;
	}
	case cr_renderer_bounces: {
		if (num > 512) return false;
		r->prefs.bounces = num;
		return true;
	}
	case cr_renderer_tile_width: {
		
	}
	}
	return false;
}

uint64_t cr_renderer_get_num_pref(struct cr_renderer *r, enum cr_renderer_param p) {
	//TODO
	(void)r;
	(void)p;
	return 0;
}

void cr_destroy_renderer(struct cr_renderer *ext) {
	struct renderer *r = (struct renderer *)ext;
	ASSERT(r);
	destroyRenderer(r);
}

// -- Scene --

struct cr_scene;

struct cr_scene *cr_scene_create(struct cr_renderer *r) {
	(void)r;
	return NULL;
}

struct cr_object;

struct cr_object *cr_object_new(struct cr_scene *s) {
	(void)s;
	return NULL;
}

struct cr_instance;

struct cr_instance *cr_instance_new(struct cr_object *o) {
	return NULL;
}

// -- Camera --

struct cr_camera;

struct cr_camera *cr_camera_new(struct cr_scene *s) {
	//TODO
	(void)s;
	return NULL;
}

bool cr_camera_remove(struct cr_scene *s, struct cr_camera *c) {
	//TODO
	(void)s;
	(void)c;
	return false;
}

// --

void cr_write_image(struct cr_renderer *ext) {
	struct renderer *r = (struct renderer *)ext;
	if (currentImage) {
		if (r->state.saveImage) {
			struct imageFile *file = newImageFile(currentImage, r->prefs.imgFilePath, r->prefs.imgFileName, r->prefs.imgCount, r->prefs.imgType);
			file->info = (struct renderInfo){
				.bounces = cr_get_bounces(r),
				.samples = cr_get_sample_count(r),
				.crayVersion = cr_get_version(),
				.gitHash = cr_get_git_hash(),
				.renderTime = timer_get_ms(r->state.timer),
				.threadCount = cr_get_thread_count(r)
			};
			writeImage(file);
			destroyImageFile(file);
		} else {
			logr(info, "Abort pressed, image won't be saved.\n");
		}
	}
}

int cr_load_scene_from_file(struct cr_renderer *ext, char *file_path) {
	struct renderer *r = (struct renderer *)ext;
	size_t bytes = 0;
	char *input = load_file(file_path, &bytes, NULL);
	if (input) {
		if (loadScene(r, file_path) != 0) {
			return -1;
		}
	} else {
		return -1;
	}
	return 0;
}

void cr_load_mesh_from_file(char *file_path) {
	(void)file_path;
	ASSERT_NOT_REACHED();
}

void cr_load_mesh_from_buf(char *buf) {
	(void)buf;
	ASSERT_NOT_REACHED();
}

int cr_load_scene_from_buf(struct cr_renderer *ext, char *buf) {
	struct renderer *r = (struct renderer *)ext;
	return loadScene(r, buf);
}

int cr_get_thread_count(struct cr_renderer *ext) {
	struct renderer *r = (struct renderer *)ext;
	return r->prefs.threads;
}

int cr_get_sample_count(struct cr_renderer *ext) {
	struct renderer *r = (struct renderer *)ext;
	return r->prefs.sampleCount;
}

int cr_get_bounces(struct cr_renderer *ext) {
	struct renderer *r = (struct renderer *)ext;
	return r->prefs.bounces;
}

void cr_set_asset_path(struct cr_renderer *ext) {
	struct renderer *r = (struct renderer *)ext;
	r->prefs.assetPath = cr_is_option_set("inputFile") ? cr_get_file_path(cr_path_arg()) : cr_is_option_set("asset_path") ? stringCopy(specifiedAssetPath()) : stringCopy("./");
}

void cr_start_renderer(struct cr_renderer *ext) {
	struct renderer *r = (struct renderer *)ext;
	if (isSet("use_clustering")) {
		r->prefs.useClustering = true;
		r->state.clients = syncWithClients(r, &r->state.clientCount);
		free(r->sceneCache);
		r->sceneCache = NULL;
		cache_destroy(r->state.file_cache);
	}
	if (!r->state.clients && r->prefs.threads == 0) {
		logr(warning, "You specified 0 local threads, and no network clients were found. Nothing to do.\n");
		return;
	}
	timer_start(&r->state.timer);
	currentImage = renderFrame(r);
	long ms = timer_get_ms(r->state.timer);
	logr(info, "Finished render in ");
	printSmartTime(ms);
	logr(plain, "                     \n");
}

void cr_start_render_worker() {
	startWorkerServer();
}

