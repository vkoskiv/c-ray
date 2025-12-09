//
//  main.c
//  c-ray
//
//  Created by Valtteri Koskivuori on 12/02/2015.
//  Copyright © 2015-2023 Valtteri Koskivuori. All rights reserved.
//

#include <c-ray/c-ray.h>
#include <v.h>

#include <imagefile.h>
#include <common/logging.h>
#include <common/cr_string.h>
#include <common/fileio.h>
#include <common/platform/terminal.h>
#include <common/hashtable.h>
#include <common/vendored/cJSON.h>
#include <common/json_loader.h>
#include <encoders/encoder.h>
#include <args.h>
#include <sdl.h>

struct cb_context {
	struct cr_renderer *r;
	struct sdl_window *w;
	bool skip_save;

	const struct cr_bitmap **temp_rbuf;
};

/*
	This hack exists to deal with the niche issue that SDL_CreateWindow() in
	win_try_init() blocks if c-ray is started from a full-screen terminal on i3.
	We delegate window initialization to a background thread so we don't needlessly
	wait around if that happens, and SDL init is fairly slow anyhow, so this speeds
	up init by up to ~250ms.
	To clarify, on_start() is a callback we register in main() that the renderer calls
	when it has allocated a render buffer and is ready to render. We then pass a a ref
	to that buffer and some context to this async win_init_task().
*/
static void *win_init_task(void *arg) {
	struct cb_context *d = arg;
	d->w = win_try_init(d->temp_rbuf);
	d->temp_rbuf = NULL;
	return NULL;
}

static void on_start(struct cr_renderer_cb_info *cb_info, void *user_data) {
	struct cb_context *d = user_data;
	d->temp_rbuf = cb_info->fb;
#if defined(__APPLE__)
	// The SDL2 Cocoa backend on macOS will crash the program if it's invoked from a background thread.
	// Solution is to init SDL2 synchronously on macOS.
	win_init_task(d);;
#else
	v_thread_ctx ctx = {
		.thread_fn = win_init_task,
		.ctx = d,
	};
	v_thread *ret = v_thread_create(ctx, v_thread_type_detached);
	if (!ret) // Fall back to running synchronously.
		win_init_task(d);
#endif
}

static void on_stop(struct cr_renderer_cb_info *info, void *user_data) {
	(void)info;
	struct cb_context *d = user_data;
	if (d->w)
		win_destroy(d->w);
	if (info->aborted)
		d->skip_save = true;
}

static void status(struct cr_renderer_cb_info *state, void *user_data) {
	static int pauser = 0;
	struct cb_context *d = user_data;
	if (!d)
		return;
	if (d->w) {
		enum input_event e = win_update(d->w, state->tiles, state->tiles_count);
		switch (e) {
		case ev_stop_nosave:
			d->skip_save = true;
			cr_renderer_stop(d->r);
			break;
		case ev_stop:
			cr_renderer_stop(d->r);
			break;
		case ev_pause:
			cr_renderer_toggle_pause(d->r);
		default:
			break;
		}
	}

	//Run the status printing about 4x/s
	if (++pauser >= 16) {
		pauser = 0;
		char rem[64];
		ms_to_readable(state->eta_ms, rem);
		logr(info, "[%s%.0f%%%s] μs/path: %.02f, etf: %s, %.02lfMs/s %s        \r",
			KBLU,
			state->completion * 100.0,
			KNRM,
			state->avg_per_ray_us,
			rem,
			0.000001 * (double)state->samples_per_sec,
			state->paused ? "[PAUSED]" : "");
	}
}

int main(int argc, char *argv[]) {
	term_init();
	atexit(term_restore);
	logr(info, "c-ray v%s [%.8s], © 2015-2025 Valtteri Koskivuori\n", cr_get_version(), cr_get_git_hash());

	struct driver_args *opts = args_parse(argc, argv);
	char *log_level = args_string(opts, "log_level");
	if (stringEquals(log_level, "debug")) {
		cr_log_level_set(Debug);
	} else if (stringEquals(log_level, "spam")) {
		cr_log_level_set(Spam);
	}
	if (args_is_set(opts, "is_worker")) {
		int port = args_is_set(opts, "worker_port") ? args_int(opts, "worker_port") : C_RAY_PROTO_DEFAULT_PORT;
		size_t thread_limit = 0;
		if (args_is_set(opts, "thread_override")) thread_limit = args_int(opts, "thread_override");
		cr_start_render_worker(port, thread_limit);
		args_destroy(opts);
		return 0;
	}
	
	struct cr_renderer *renderer = cr_new_renderer();

	if (args_is_set(opts, "asset_path")) {
		cr_renderer_set_str_pref(renderer, cr_renderer_asset_path, args_asset_path(opts));
	} else if (args_is_set(opts, "inputFile")) {
		char *asset_path = get_file_path(args_path(opts));
		cr_renderer_set_str_pref(renderer, cr_renderer_asset_path, asset_path);
		free(asset_path);
	}

	int ret = 0;
	file_data input_bytes = args_is_set(opts, "inputFile") ? file_load(args_path(opts)) : read_stdin();
	if (!v_arr_len(input_bytes)) {
		logr(info, "No input provided, exiting.\n");
		ret = -1;
		goto done;
	}
	char size_buf[64];
	logr(info, "%s of input JSON loaded from %s, parsing.\n", human_file_size(v_arr_len(input_bytes), size_buf), args_is_set(opts, "inputFile") ? "file" : "stdin");
	v_timer json_timer = v_timer_start();
	cJSON *input_json = cJSON_ParseWithLength((const char *)input_bytes, v_arr_len(input_bytes));
	size_t json_ms = v_timer_get_ms(json_timer);
	if (!input_json) {
		const char *errptr = cJSON_GetErrorPtr();
		if (errptr) {
			logr(warning, "Failed to parse JSON\n");
			logr(warning, "Error before: %s\n", errptr);
			goto done;
		}
	}
	logr(info, "JSON parse took %zums\n", json_ms);

	v_arr_free(input_bytes);

	if (args_is_set(opts, "nodes_list")) {
		cr_renderer_set_str_pref(renderer, cr_renderer_node_list, args_string(opts, "nodes_list"));
	}

	if (parse_json(renderer, input_json) < 0) {
		logr(warning, "Scene parse failed, exiting.\n");
		ret = -1;
		goto done;
	}

	if (args_is_set(opts, "cam_index")) {
		cr_renderer_set_num_pref(renderer, cr_renderer_override_cam, args_int(opts, "cam_index"));
	}

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
			logr(warning, "Can't use interactive mode with network rendering yet, sorry.\n");
		} else {
			cr_renderer_set_num_pref(renderer, cr_renderer_is_interactive, 1);
		}
	}
	
	struct cb_context usrdata = {
		.r = renderer,
	};

	const cJSON *display = cJSON_GetObjectItem(input_json, "display");
	if (!args_is_set(opts, "no_sdl") && display && !cJSON_IsFalse(cJSON_GetObjectItem(display, "enabled")))
		cr_renderer_set_callback(renderer, cr_cb_on_start, on_start, &usrdata);

	cr_renderer_set_callback(renderer, cr_cb_on_stop, on_stop, &usrdata);
	cr_renderer_set_callback(renderer, cr_cb_status_update, status, &usrdata);

	const cJSON *r = cJSON_GetObjectItem(input_json, "renderer");
	const cJSON *count = cJSON_GetObjectItem(r, "count");
	char *output_path = NULL;
	char *output_name = NULL;
	const cJSON *path = cJSON_GetObjectItem(r, "outputFilePath");
	const cJSON *name = cJSON_GetObjectItem(r, "outputFileName");
	char *arg_path = NULL;
	if (args_is_set(opts, "output_path")) {
		char *arg_path = args_string(opts, "output_path");
		logr(info, "Overriding output path to %s\n", arg_path);
	}
	char *temp_path = get_file_path(arg_path);
	char *temp_name = get_file_name(arg_path);
	output_path = temp_path ? stringCopy(temp_path) : cJSON_IsString(path) ? stringCopy(path->valuestring) : NULL;
	output_name = temp_name ? stringCopy(temp_name) : cJSON_IsString(name) ? stringCopy(name->valuestring) : NULL;

	uint64_t out_num = cJSON_IsNumber(count) ? count->valueint : 0;
	const cJSON *file_type = cJSON_GetObjectItem(r, "fileType");
	enum fileType output_type = match_file_type(cJSON_GetStringValue(file_type));

	logr(debug, "Deleting JSON...\n");
	cJSON_Delete(input_json);
	logr(debug, "Deleting done\n");

	uint64_t threads = cr_renderer_get_num_pref(renderer, cr_renderer_threads);
	uint64_t width   = cr_renderer_get_num_pref(renderer, cr_renderer_override_width);
	uint64_t height  = cr_renderer_get_num_pref(renderer, cr_renderer_override_height);
	uint64_t samples = cr_renderer_get_num_pref(renderer, cr_renderer_samples);
	uint64_t bounces = cr_renderer_get_num_pref(renderer, cr_renderer_bounces);

	logr(info, "Starting c-ray renderer for frame %"PRIu64"\n", out_num);
	bool sys_thread = threads == (size_t)v_sys_get_cores() + 2;
	logr(info, "Rendering at %s%"PRIu64"%s x %s%"PRIu64"%s\n", KWHT, width, KNRM, KWHT, height, KNRM);
	logr(info, "Rendering %s%"PRIu64"%s samples with %s%"PRIu64"%s bounces.\n", KBLU, samples, KNRM, KGRN, bounces, KNRM);
	logr(info, "Rendering with %s%"PRIu64"%s%s local thread%s.\n",
		KRED,
		sys_thread ? threads - 2 : threads,
		sys_thread ? "+2" : "",
		KNRM,
		PLURAL(threads));

	v_timer timer = v_timer_start();
	cr_renderer_render(renderer);
	long ms = v_timer_get_ms(timer);
	char buf[64] = { 0 };
	logr(plain, "\n");
	logr(info, "Finished render in %s\n", ms_to_readable(ms, buf));

	if (usrdata.skip_save) {
		logr(info, "Abort pressed, image won't be saved.\n");
	} else {
		struct imageFile file = (struct imageFile){
			.filePath = output_path,
			.fileName = output_name,
			.count = out_num,
			.type = output_type,
			.info = {
				.bounces = cr_renderer_get_num_pref(renderer, cr_renderer_bounces),
				.samples = cr_renderer_get_num_pref(renderer, cr_renderer_samples),
				.crayVersion = cr_get_version(),
				.gitHash = cr_get_git_hash(),
				.renderTime = ms,
				.threadCount = cr_renderer_get_num_pref(renderer, cr_renderer_threads)
			},
			.t = cr_renderer_get_result(renderer)
		};
		writeImage(&file);
		logr(info, "Render finished, exiting.\n");
	}
	
	if (output_path) free(output_path);
	if (output_name) free(output_name);

done:
	cr_destroy_renderer(renderer);
	args_destroy(opts);
	return ret;
}
