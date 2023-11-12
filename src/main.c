//
//  main.c
//  C-ray
//
//  Created by Valtteri Koskivuori on 12/02/2015.
//  Copyright © 2015-2023 Valtteri Koskivuori. All rights reserved.
//

#include <stdlib.h>
#include <c-ray/c-ray.h>

#include "utils/logging.h"
#include "utils/fileio.h"
#include "utils/args.h"
#include "utils/platform/terminal.h"

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
	if (cr_load_scene_from_buf(renderer, input) < 0) {
		logr(warning, "Scene parse failed, exiting.\n");
		ret = -1;
		goto done;
	}

	cr_start_renderer(renderer);
	cr_write_image(renderer);
	
done:
	cr_destroy_renderer(renderer);
	args_destroy();
	logr(info, "Render finished, exiting.\n");
	return ret;
}
