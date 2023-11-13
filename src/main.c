//
//  main.c
//  c-ray
//
//  Created by Valtteri Koskivuori on 12/02/2015.
//  Copyright © 2015-2023 Valtteri Koskivuori. All rights reserved.
//

#include <stdlib.h>
#include <c-ray/c-ray.h>

#include "vendored/cJSON.h"

#include "utils/logging.h"
#include "utils/fileio.h"
#include "utils/args.h"
#include "utils/platform/terminal.h"
#include "utils/loaders/sceneloader.h"
#include "datatypes/image/imagefile.h"
#include "utils/encoders/encoder.h"
#include "utils/timer.h"

int main(int argc, char *argv[]) {
	term_init();
	atexit(term_restore);
	logr(info, "c-ray v%s [%.8s], © 2015-2023 Valtteri Koskivuori\n", cr_get_version(), cr_get_git_hash());
	args_parse(argc, argv);
	struct cr_renderer *renderer = cr_new_renderer();

	if (args_is_set("asset_path")) {
		cr_renderer_set_str_pref(renderer, cr_renderer_asset_path, args_asset_path());
	} else if (args_is_set("inputFile")) {
		cr_renderer_set_str_pref(renderer, cr_renderer_asset_path, get_file_path(args_path()));
	}

	int ret = 0;
	if (args_is_set("is_worker")) {
		cr_start_render_worker();
		goto done;
	}

	size_t bytes = 0;
	char *input = args_is_set("inputFile") ? load_file(args_path(), &bytes, NULL) : read_stdin(&bytes);
	if (!input) {
		logr(info, "No input provided, exiting.\n");
		ret = -1;
		goto done;
	}
	logr(info, "%zi bytes of input JSON loaded from %s, parsing.\n", bytes, args_is_set("inputFile") ? "file" : "stdin");
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

	if (parse_json(renderer, scene) < 0) {
		logr(warning, "Scene parse failed, exiting.\n");
		ret = -1;
		goto done;
	}

	// FIXME: Remove global options table, store it in a local in main() and run overrides
	// from there.

	// Now check and apply potential CLI overrides.
	if (args_is_set("thread_override")) {
		size_t threads = args_int("thread_override");
		int64_t curr = cr_renderer_get_num_pref(renderer, cr_renderer_threads);
		if (curr != (int64_t)threads) {
			logr(info, "Overriding thread count to %zu\n", threads);
			cr_renderer_set_num_pref(renderer, cr_renderer_threads, threads);
			// prefs->fromSystem = false; FIXME
		}
	}
	
	if (args_is_set("samples_override")) {
		if (args_is_set("is_worker")) {
			logr(warning, "Can't override samples when in worker mode\n");
		} else {
			int samples = args_int("samples_override");
			logr(info, "Overriding sample count to %i\n", samples);
			cr_renderer_set_num_pref(renderer, cr_renderer_samples, samples);
		}
	}
	
	if (args_is_set("dims_override")) {
		if (args_is_set("is_worker")) {
			logr(warning, "Can't override dimensions when in worker mode\n");
		} else {
			int width = args_int("dims_width");
			int height = args_int("dims_height");
			logr(info, "Overriding image dimensions to %ix%i\n", width, height);
			cr_renderer_set_num_pref(renderer, cr_renderer_override_width, width);
			cr_renderer_set_num_pref(renderer, cr_renderer_override_height, height);
		}
	}
	
	if (args_is_set("tiledims_override")) {
		if (args_is_set("is_worker")) {
			logr(warning, "Can't override tile dimensions when in worker mode\n");
		} else {
			int width = args_int("tile_width");
			int height = args_int("tile_height");
			logr(info, "Overriding tile  dimensions to %ix%i\n", width, height);
			cr_renderer_set_num_pref(renderer, cr_renderer_tile_width, width);
			cr_renderer_set_num_pref(renderer, cr_renderer_tile_height, height);
		}
	}

	if (args_is_set("cam_index")) {
		cr_renderer_set_num_pref(renderer, cr_renderer_override_cam, args_int("cam_index"));
	}

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
	if (args_is_set("output_path")) {
		char *path = args_string("output_path");
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
	args_destroy();
	logr(info, "Render finished, exiting.\n");
	return ret;
}
