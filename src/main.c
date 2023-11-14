//
//  main.c
//  c-ray
//
//  Created by Valtteri Koskivuori on 12/02/2015.
//  Copyright © 2015-2023 Valtteri Koskivuori. All rights reserved.
//

#include <c-ray/c-ray.h>

#include "datatypes/image/texture.h"
#include "vendored/cJSON.h"

#include "utils/logging.h"
#include "utils/fileio.h"
#include "utils/args.h"
#include "utils/platform/terminal.h"
#include "utils/loaders/sceneloader.h"
#include "datatypes/image/imagefile.h"
#include "utils/encoders/encoder.h"
#include "utils/timer.h"
#include "utils/hashtable.h"
#include "utils/platform/sdl.h"

struct usr_data {
	struct cr_renderer *r;
	struct sdl_window *w;
	struct sdl_prefs p;
};

static void on_start(struct cr_renderer_cb_info *info) {
	struct usr_data *d = info->user_data;
	if (d->p.enabled && info->fb) d->w = win_try_init(&d->p, info->fb->width, info->fb->height);
}

static void on_stop(struct cr_renderer_cb_info *info) {
	struct usr_data *d = info->user_data;
	if (d->w) win_destroy(d->w);
}

static void status(struct cr_renderer_cb_info *info) {
	struct usr_data *d = info->user_data;
	if (!d || !d->w) return;
	struct input_state in = win_update(d->w, info->tiles, info->tiles_count, info->fb);
	if (in.stop_render) cr_renderer_stop(d->r, in.should_save);
	if (in.pause_render) cr_renderer_toggle_pause(d->r);
}

int main(int argc, char *argv[]) {
	term_init();
	atexit(term_restore);
	logr(info, "c-ray v%s [%.8s], © 2015-2023 Valtteri Koskivuori\n", cr_get_version(), cr_get_git_hash());

	struct driver_args *opts = args_parse(argc, argv);
	
	if (args_is_set(opts, "v")) log_toggle_verbose();
	
	struct cr_renderer *renderer = cr_new_renderer();

	if (args_is_set(opts, "asset_path")) {
		cr_renderer_set_str_pref(renderer, cr_renderer_asset_path, args_asset_path(opts));
	} else if (args_is_set(opts, "inputFile")) {
		cr_renderer_set_str_pref(renderer, cr_renderer_asset_path, get_file_path(args_path(opts)));
	}

	int ret = 0;
	if (args_is_set(opts, "is_worker")) {
		int port = args_is_set(opts, "worker_port") ? args_int(opts, "worker_port") : C_RAY_PROTO_DEFAULT_PORT;
		cr_start_render_worker(port);
		goto done;
	}

	size_t bytes = 0;
	char *input = args_is_set(opts, "inputFile") ? load_file(args_path(opts), &bytes, NULL) : read_stdin(&bytes);
	if (!input) {
		logr(info, "No input provided, exiting.\n");
		ret = -1;
		goto done;
	}
	logr(info, "%zi bytes of input JSON loaded from %s, parsing.\n", bytes, args_is_set(opts, "inputFile") ? "file" : "stdin");
	cJSON *scene = cJSON_Parse(input);
	if (!scene) {
		const char *errptr = cJSON_GetErrorPtr();
		if (errptr) {
			logr(warning, "Failed to parse JSON\n");
			logr(warning, "Error before: %s\n", errptr);
			goto done;
		}
	}

	//FIXME: mmap() input
	free(input);

	if (args_is_set(opts, "nodes_list")) {
		cr_renderer_set_str_pref(renderer, cr_renderer_node_list, args_string(opts, "nodes_list"));
	}

	if (parse_json(renderer, scene) < 0) {
		logr(warning, "Scene parse failed, exiting.\n");
		ret = -1;
		goto done;
	}

	if (args_is_set(opts, "cam_index")) {
		cr_renderer_set_num_pref(renderer, cr_renderer_override_cam, args_int(opts, "cam_index"));
	}

	// FIXME: Remove global options table, store it in a local in main() and run overrides
	// from there.

	// Now check and apply potential CLI overrides.
	if (args_is_set(opts, "thread_override")) {
		size_t threads = args_int(opts, "thread_override");
		int64_t curr = cr_renderer_get_num_pref(renderer, cr_renderer_threads);
		if (curr != (int64_t)threads) {
			logr(info, "Overriding thread count to %zu\n", threads);
			cr_renderer_set_num_pref(renderer, cr_renderer_threads, threads);
			// prefs->fromSystem = false; FIXME
		}
	}
	
	if (args_is_set(opts, "samples_override")) {
		if (args_is_set(opts, "is_worker")) {
			logr(warning, "Can't override samples when in worker mode\n");
		} else {
			int samples = args_int(opts, "samples_override");
			logr(info, "Overriding sample count to %i\n", samples);
			cr_renderer_set_num_pref(renderer, cr_renderer_samples, samples);
		}
	}
	
	if (args_is_set(opts, "dims_override")) {
		if (args_is_set(opts, "is_worker")) {
			logr(warning, "Can't override dimensions when in worker mode\n");
		} else {
			int width = args_int(opts, "dims_width");
			int height = args_int(opts, "dims_height");
			logr(info, "Overriding image dimensions to %ix%i\n", width, height);
			cr_renderer_set_num_pref(renderer, cr_renderer_override_width, width);
			cr_renderer_set_num_pref(renderer, cr_renderer_override_height, height);
		}
	}
	
	if (args_is_set(opts, "tiledims_override")) {
		if (args_is_set(opts, "is_worker")) {
			logr(warning, "Can't override tile dimensions when in worker mode\n");
		} else {
			int width = args_int(opts, "tile_width");
			int height = args_int(opts, "tile_height");
			logr(info, "Overriding tile  dimensions to %ix%i\n", width, height);
			cr_renderer_set_num_pref(renderer, cr_renderer_tile_width, width);
			cr_renderer_set_num_pref(renderer, cr_renderer_tile_height, height);
		}
	}

	if (args_is_set(opts, "interactive")) {
		if (args_is_set(opts, "nodes_list")) {
			logr(warning, "Can't use iterative mode with network rendering yet, sorry.\n");
		} else {
			cr_renderer_set_num_pref(renderer, cr_renderer_is_iterative, 1);
		}
	}
	
	// This is where we prepare a cache of scene data to be sent to worker nodes
	// We also apply any potential command-line overrides to that cache here as well.
	// FIXME: This overrides setting should be integrated with scene loading, probably.
	if (args_is_set(opts, "nodes_list")) {
		// Stash a cache of scene data here
		// Apply overrides to the cache here
		if (args_is_set(opts, "samples_override")) {
			cJSON *renderer = cJSON_GetObjectItem(scene, "renderer");
			if (cJSON_IsObject(renderer)) {
				int samples = args_int(opts, "samples_override");
				logr(debug, "Overriding cache sample count to %i\n", samples);
				if (cJSON_IsNumber(cJSON_GetObjectItem(renderer, "samples"))) {
					cJSON_ReplaceItemInObject(renderer, "samples", cJSON_CreateNumber(samples));
				} else {
					cJSON_AddItemToObject(renderer, "samples", cJSON_CreateNumber(samples));
				}
			}
		}

		if (args_is_set(opts, "dims_override")) {
			cJSON *renderer = cJSON_GetObjectItem(scene, "renderer");
			if (cJSON_IsObject(renderer)) {
				int width = args_int(opts, "dims_width");
				int height = args_int(opts, "dims_height");
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

		if (args_is_set(opts, "tiledims_override")) {
			cJSON *renderer = cJSON_GetObjectItem(scene, "renderer");
			if (cJSON_IsObject(renderer)) {
				int width = args_int(opts, "tile_width");
				int height = args_int(opts, "tile_height");
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

		if (args_is_set(opts, "cam_index")) {
			cJSON_AddItemToObject(scene, "selected_camera", cJSON_CreateNumber(args_int(opts, "cam_index")));
		}

		// Store cache. This is what gets sent to worker nodes.
		char *cache = cJSON_PrintUnformatted(scene);
		cr_renderer_set_str_pref(renderer, cr_renderer_scene_cache, cache);
		free(cache);
	}

	cr_renderer_set_callbacks(renderer, (struct cr_renderer_callbacks){
		.cr_renderer_on_start = on_start,
		.cr_renderer_on_stop = on_stop,
		.cr_renderer_status = status,
		.user_data = &(struct usr_data){
			.p = sdl_parse(cJSON_GetObjectItem(scene, "display")),
			.r = renderer
		}
	});

	logr(debug, "Deleting JSON...\n");
	cJSON_Delete(scene);
	logr(debug, "Deleting done\n");

	struct timeval timer;
	timer_start(&timer);
	struct texture *final = cr_renderer_render(renderer);
	long ms = timer_get_ms(timer);
	logr(info, "Finished render in ");
	printSmartTime(ms);
	logr(plain, "                     \n");

	// FIXME: What the fuck
	const char *output_path = NULL;
	const char *output_name = NULL;
	if (args_is_set(opts, "output_path")) {
		char *path = args_string(opts, "output_path");
		logr(info, "Overriding output path to %s\n", path);
		char *temp_path = get_file_path(path);
		char *temp_name = get_file_name(path);
		output_path = temp_path ? temp_path : cr_renderer_get_str_pref(renderer, cr_renderer_output_path);
		output_name = temp_name ? temp_name : cr_renderer_get_str_pref(renderer, cr_renderer_output_name);
	} else {
		output_path = cr_renderer_get_str_pref(renderer, cr_renderer_output_path);
		output_name = cr_renderer_get_str_pref(renderer, cr_renderer_output_name);
	}

	if (cr_renderer_get_num_pref(renderer, cr_renderer_should_save)) {
		struct imageFile file = (struct imageFile){
			.filePath = output_path,
			.fileName = output_name,
			.count =  cr_renderer_get_num_pref(renderer, cr_renderer_output_num),
			.type = cr_renderer_get_num_pref(renderer, cr_renderer_output_filetype),
			.info = {
				.bounces = cr_renderer_get_num_pref(renderer, cr_renderer_bounces),
				.samples = cr_renderer_get_num_pref(renderer, cr_renderer_samples),
				.crayVersion = cr_get_version(),
				.gitHash = cr_get_git_hash(),
				.renderTime = ms,
				.threadCount = cr_renderer_get_num_pref(renderer, cr_renderer_threads)
			},
			.t = final
		};
		writeImage(&file);
	} else {
		logr(info, "Abort pressed, image won't be saved.\n");
	}
	
done:
	cr_destroy_renderer(renderer);
	args_destroy(opts);
	logr(info, "Render finished, exiting.\n");
	return ret;
}
