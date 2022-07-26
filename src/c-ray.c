//
//  c-ray.c
//  C-ray
//
//  Created by Valtteri on 5.1.2020.
//  Copyright © 2020-2022 Valtteri Koskivuori. All rights reserved.
//

#include "includes.h"
#include "c-ray.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdbool.h>

#include "datatypes/image/imagefile.h"
#include "renderer/renderer.h"
#include "datatypes/scene.h"
#include "utils/gitsha1.h"
#include "utils/logging.h"
#include "utils/fileio.h"
#include "utils/platform/terminal.h"
#include "utils/assert.h"
#include "datatypes/image/texture.h"
#include "utils/ui.h"
#include "utils/timer.h"
#include "utils/args.h"
#include "utils/encoders/encoder.h"
#include "utils/string.h"
#include "utils/protocol/server.h"
#include "utils/protocol/worker.h"
#include "utils/filecache.h"
#include "utils/hashtable.h"
#include "datatypes/camera.h"

#define VERSION "0.6.3"

struct texture *currentImage = NULL;

char *cr_get_version() {
	return VERSION;
}

char *cr_get_git_hash() {
	return gitHash();
}

int is_debug() {
#ifdef CRAY_DEBUG_ENABLED
	return true;
#else
	return false;
#endif
}

void at_exit() {
	restoreTerminal();
}

void cr_initialize() {
	initTerminal();
	atexit(at_exit);
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
	return getFilePath(full_path);
}

void cr_write_image(struct renderer *r) {
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

char *cr_read_from_file(size_t *bytes) {
	return loadFile(pathArg(), bytes, NULL);
}

char *cr_read_from_stdin(size_t *bytes) {
	return readStdin(bytes);
}

struct renderer *cr_new_renderer() {
	struct renderer *r = newRenderer();
	cr_set_asset_path(r);
	return r;
}

void cr_destroy_renderer(struct renderer *r) {
	ASSERT(r);
	destroyRenderer(r);
}

int cr_load_scene_from_file(struct renderer *r, char *file_path) {
	size_t bytes = 0;
	char *input = loadFile(file_path, &bytes, NULL);
	if (input) {
		if (loadScene(r, file_path) != 0) {
			free(input);
			return -1;
		}
	} else {
		return -1;
	}
	free(input);
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

int cr_load_scene_from_buf(struct renderer *r, char *buf) {
	return loadScene(r, buf);
}

void cr_log(const char *fmt, ...) {
	char buf[512];
	va_list vl;
	va_start(vl, fmt);
	vsnprintf(buf, sizeof(buf), fmt, vl);
	va_end(vl);
	logr(info, "%s", buf);
}

void cr_set_render_order(void) {
	ASSERT_NOT_REACHED();
}

void cr_get_render_order(void) {
	ASSERT_NOT_REACHED();
}

void cr_set_thread_count(struct renderer *r, int thread_count, int is_from_system) {
	r->prefs.threadCount = thread_count;
	r->prefs.fromSystem = is_from_system;
	cr_restart_interactive();
}

int cr_get_thread_count(struct renderer *r) {
	return r->prefs.threadCount;
}

void cr_set_sample_count(struct renderer *r, int sample_count) {
	ASSERT(sample_count > 0);
	r->prefs.sampleCount = sample_count;
}

int cr_get_sample_count(struct renderer *r) {
	return r->prefs.sampleCount;
}

void cr_set_bounces(struct renderer *r, int bounces) {
	ASSERT(bounces > 0);
	r->prefs.bounces = bounces;
}

int cr_get_bounces(struct renderer *r) {
	return r->prefs.bounces;
}

unsigned cr_get_tile_width(struct renderer *r) {
	return r->prefs.tileWidth;
}

unsigned cr_get_tile_height(struct renderer *r) {
	return r->prefs.tileHeight;
}

unsigned cr_get_image_width(struct renderer *r) {
	return r->scene->cameras[r->prefs.selected_camera].width;
}

unsigned cr_get_image_height(struct renderer *r) {
	return r->scene->cameras[r->prefs.selected_camera].height;
}

void cr_set_output_path(struct renderer *r, char *file_path) {
	r->prefs.imgFilePath = file_path;
}

char *cr_get_output_path(struct renderer *r) {
	return r->prefs.imgFilePath;
}

void cr_set_file_name(struct renderer *r, char *file_name) {
	(void)r;
	(void)file_name;
	ASSERT_NOT_REACHED();
}

char *cr_get_file_name(struct renderer *r) {
	return r->prefs.imgFileName;
}

void cr_set_asset_path(struct renderer *r) {
	r->prefs.assetPath = cr_is_option_set("inputFile") ? cr_get_file_path(cr_path_arg()) : cr_is_option_set("asset_path") ? specifiedAssetPath() : stringCopy("./");
}

char *cr_get_asset_path(struct renderer *r) {
	return r->prefs.assetPath;
}

void cr_start_renderer(struct renderer *r) {
	if (isSet("use_clustering")) {
		r->prefs.useClustering = true;
		r->state.clients = syncWithClients(r, &r->state.clientCount);
		free(r->sceneCache);
		r->sceneCache = NULL;
		cache_destroy(r->state.file_cache);
	}
	if (!r->state.clients && r->prefs.threadCount == 0) {
		logr(warning, "You specified 0 local threads, and no network clients were found. Nothing to do.\n");
		return;
	}
	struct camera cam = r->scene->cameras[r->prefs.selected_camera];
	if (r->prefs.enabled) initDisplay(r->prefs.fullscreen, r->prefs.borderless, cam.width, cam.height, r->prefs.scale);
	timer_start(&r->state.timer);
	currentImage = renderFrame(r);
	printDuration(timer_get_ms(r->state.timer));
	if (r->prefs.enabled) destroyDisplay();
}

void cr_start_render_worker() {
	startWorkerServer();
}

//Interactive mode
void cr_start_interactive(void) {
	ASSERT_NOT_REACHED();
}

//Toggle paused state
void cr_pause_interactive(void) {
	ASSERT_NOT_REACHED();
}

//Just get the current buffer
void cr_get_current_image(void) {
	ASSERT_NOT_REACHED();
}
void cr_restart_interactive() {
	//if (grenderer->prefs.interactive) { do the thing }
	ASSERT_NOT_REACHED();
}

void cr_transform_mesh(void); //Transform, recompute kd-tree, restart

void cr_move_camera(void/*struct dimension delta*/);
void cr_set_hdr(void);
