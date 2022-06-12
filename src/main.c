//
//  main.c
//  C-ray
//
//  Created by Valtteri Koskivuori on 12/02/2015.
//  Copyright © 2015-2022 Valtteri Koskivuori. All rights reserved.
//

#include <stdlib.h>

#include "c-ray.h"

int main(int argc, char *argv[]) {
	cr_log("c-ray v%s%s [%.8s], © 2015-2022 Valtteri Koskivuori\n", cr_get_version(), is_debug() ? "D" : "", cr_get_git_hash());
	cr_initialize();
	cr_parse_args(argc, argv);
	struct renderer *renderer = cr_new_renderer();
	if (!cr_is_option_set("is_worker")) {
		size_t bytes = 0;
		char *input = cr_is_option_set("inputFile") ? cr_read_from_file(&bytes) : cr_read_from_stdin(&bytes);
		if (!input) {
			cr_log("No input provided, exiting.\n");
			cr_destroy_renderer(renderer);
			cr_destroy_options();
			return -1;
		}
		cr_log("%zi bytes of input JSON loaded from %s, parsing.\n", bytes, cr_is_option_set("inputFile") ? "file" : "stdin");
		if (cr_load_scene_from_buf(renderer, input) < 0) {
			cr_log("Scene parse failed, exiting.\n");
			free(input);
			cr_destroy_renderer(renderer);
			cr_destroy_options();
			return 0;
		}
		free(input);

		cr_start_renderer(renderer);
		cr_write_image(renderer);
	} else {
		cr_start_render_worker();
	}
	
	cr_destroy_renderer(renderer);
	cr_destroy_options();
	cr_log("Render finished, exiting.\n");
	return 0;
}
