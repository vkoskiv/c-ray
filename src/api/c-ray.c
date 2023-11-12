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

char *cr_get_file_path(char *full_path) {
	return get_file_path(full_path);
}

// -- Renderer --

struct cr_renderer;

struct cr_renderer *cr_new_renderer() {
	struct renderer *r = renderer_new();
	cr_set_asset_path((struct cr_renderer *)r);
	return (struct cr_renderer *)r;
}

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
			r->prefs.tileWidth = num;
			return true;
		}
		case cr_renderer_tile_height: {
			r->prefs.tileHeight = num;
			return true;
		}
		case cr_renderer_output_num: {
			r->prefs.imgCount = num;
			return true;
		}
		case cr_renderer_override_width: {
			r->prefs.override_width = num;
			r->prefs.override_dimensions = true;
			return true;
		}
		case cr_renderer_override_height: {
			r->prefs.override_height = num;
			r->prefs.override_dimensions = true;
			return true;
		}
		case cr_renderer_override_cam: {
			r->prefs.selected_camera = num;
			return true;
		}
		default: {
			logr(warning, "Renderer param %i not a number\n", p);
		}
	}
	return false;
}

bool cr_renderer_set_str_pref(struct cr_renderer *ext, enum cr_renderer_param p, const char *str) {
	if (!ext) return false;
	struct renderer *r = (struct renderer *)ext;
	switch (p) {
		case cr_renderer_tile_order: {
			if (stringEquals(str, "random")) {
				r->prefs.tileOrder = ro_random;
			} else if (stringEquals(str, "topToBottom")) {
				r->prefs.tileOrder = ro_top_to_bottom;
			} else if (stringEquals(str, "fromMiddle")) {
				r->prefs.tileOrder = ro_from_middle;
			} else if (stringEquals(str, "toMiddle")) {
				r->prefs.tileOrder = ro_to_middle;
			} else {
				r->prefs.tileOrder = ro_normal;
			}
			return true;
		}
		case cr_renderer_output_path: {
			if (r->prefs.imgFilePath) free(r->prefs.imgFilePath);
			r->prefs.imgFilePath = stringCopy(str);
			return true;
		}
		case cr_renderer_output_name: {
			if (r->prefs.imgFileName) free(r->prefs.imgFileName);
			r->prefs.imgFileName = stringCopy(str);
			return true;
		}
		case cr_renderer_output_filetype: {
			if (stringEquals(str, "bmp")) {
				r->prefs.imgType = bmp;
			} else if (stringEquals(str, "png")) {
				r->prefs.imgType = png;
			} else if (stringEquals(str, "qoi")) {
				r->prefs.imgType = qoi;
			} else {
				return false;
			}
			return true;
		}
		default: {
			logr(warning, "Renderer param %i not a string\n", p);
		}
	}
	return false;
}

uint64_t cr_renderer_get_num_pref(struct cr_renderer *ext, enum cr_renderer_param p) {
	if (!ext) return 0;
	struct renderer *r = (struct renderer *)ext;
	switch (p) {
		case cr_renderer_threads: return r->prefs.threads;
		case cr_renderer_samples: return r->prefs.sampleCount;
		case cr_renderer_bounces: return r->prefs.bounces;
		case cr_renderer_tile_width: return r->prefs.tileWidth;
		case cr_renderer_tile_height: return r->prefs.tileHeight;
		case cr_renderer_output_num: return r->prefs.imgCount;
		case cr_renderer_override_width: return r->prefs.override_width;
		case cr_renderer_override_height: return r->prefs.override_height;
		default: return 0; // TODO
	}
	return 0;
}

void cr_destroy_renderer(struct cr_renderer *ext) {
	struct renderer *r = (struct renderer *)ext;
	ASSERT(r);
	renderer_destroy(r);
}

// -- Scene --

struct cr_scene;

struct cr_scene *cr_scene_create(struct cr_renderer *r) {
	(void)r;
	return NULL;
}

void cr_scene_destroy(struct cr_scene *s) {
	//TODO
	(void)s;
}

struct cr_object;

struct cr_object *cr_object_new(struct cr_scene *s) {
	(void)s;
	return NULL;
}

struct cr_instance;

struct cr_instance *cr_instance_new(struct cr_object *o) {
	(void)o;
	return NULL;
}

// -- Camera --

struct camera default_camera = {
	.FOV = 80.0f,
	.focus_distance = 0.0f,
	.fstops = 0.0f,
	.width = 800,
	.height = 600,
};

cr_camera cr_camera_new(struct cr_scene *ext) {
	if (!ext) return -1;
	struct world *scene = (struct world *)ext;
	return camera_arr_add(&scene->cameras, default_camera);
}

bool cr_camera_set_num_pref(struct cr_scene *ext, cr_camera c, enum cr_camera_param p, double num) {
	if (c < 0 || !ext) return false;
	struct world *scene = (struct world *)ext;
	if ((size_t)c > scene->cameras.count - 1) return false;
	struct camera *cam = &scene->cameras.items[c];
	switch (p) {
		case cr_camera_fov: {
			cam->FOV = num;
			return true;
		}
		case cr_camera_focus_distance: {
			cam->focus_distance = num;
			return true;
		}
		case cr_camera_fstops: {
			cam->fstops = num;
			return true;
		}
		case cr_camera_pose_x: {
			cam->position.x = num;
			return true;
		}
		case cr_camera_pose_y: {
			cam->position.y = num;
			return true;
		}
		case cr_camera_pose_z: {
			cam->position.z = num;
			return true;
		}
		case cr_camera_pose_roll: {
			cam->orientation.roll = num;
			return true;
		}
		case cr_camera_pose_pitch: {
			cam->orientation.pitch = num;
			return true;
		}
		case cr_camera_pose_yaw: {
			cam->orientation.yaw = num;
			return true;
		}
		case cr_camera_time: {
			cam->time = num;
			return true;
		}
		case cr_camera_res_x: {
			cam->width = num;
			return true;
		}
		case cr_camera_res_y: {
			cam->height = num;
			return true;
		}
	}

	cam_update_pose(cam, &cam->orientation, &cam->position);
	return false;
}

bool cr_camera_update(struct cr_scene *ext, cr_camera c) {
	if (c < 0 || !ext) return false;
	struct world *scene = (struct world *)ext;
	if ((size_t)c > scene->cameras.count - 1) return false;
	struct camera *cam = &scene->cameras.items[c];
	cam_update_pose(cam, &cam->orientation, &cam->position);
	cam_recompute_optics(cam);
	return true;
}

bool cr_camera_remove(struct cr_scene *s, cr_camera c) {
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
				.bounces = cr_get_bounces(ext),
				.samples = cr_get_sample_count(ext),
				.crayVersion = cr_get_version(),
				.gitHash = cr_get_git_hash(),
				.renderTime = timer_get_ms(r->state.timer),
				.threadCount = cr_get_thread_count(ext)
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
	r->prefs.assetPath = args_is_set("inputFile") ? cr_get_file_path(args_path()) : args_is_set("asset_path") ? stringCopy(args_asset_path()) : stringCopy("./");
}

void cr_start_renderer(struct cr_renderer *ext) {
	struct renderer *r = (struct renderer *)ext;
	if (args_is_set("use_clustering")) {
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

