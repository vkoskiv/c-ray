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
#include "utils/platform/terminal.h"

int main(int argc, char *argv[]) {
	term_init();
	atexit(term_restore);
	logr(info, "c-ray v%s [%.8s], © 2015-2023 Valtteri Koskivuori\n", cr_get_version(), cr_get_git_hash());
	cr_parse_args(argc, argv);
	struct cr_renderer *renderer = cr_new_renderer();
	if (!cr_is_option_set("is_worker")) {
		size_t bytes = 0;
		char *input = cr_is_option_set("inputFile") ? load_file(cr_path_arg(), &bytes, NULL) : read_stdin(&bytes);
		if (!input) {
			logr(info, "No input provided, exiting.\n");
			cr_destroy_renderer(renderer);
			cr_destroy_options();
			return -1;
		}
		logr(info, "%zi bytes of input JSON loaded from %s, parsing.\n", bytes, cr_is_option_set("inputFile") ? "file" : "stdin");
		if (cr_load_scene_from_buf(renderer, input) < 0) {
			logr(warning, "Scene parse failed, exiting.\n");
			cr_destroy_renderer(renderer);
			cr_destroy_options();
			return 0;
		}

		cr_start_renderer(renderer);
		cr_write_image(renderer);
	} else {
		cr_start_render_worker();
	}
	
	cr_destroy_renderer(renderer);
	cr_destroy_options();
	logr(info, "Render finished, exiting.\n");
	return 0;
}
