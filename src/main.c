//
//  main.c
//  C-ray
//
//  Created by Valtteri Koskivuori on 12/02/2015.
//  Copyright © 2015-2021 Valtteri Koskivuori. All rights reserved.
//

#include <stdlib.h>
#include <stdbool.h>

#include "c-ray.h"

int main(int argc, char *argv[]) {
	crLog("C-ray v%s%s [%.8s], © 2015-2022 Valtteri Koskivuori\n", crGetVersion(), isDebug() ? "D" : "", crGitHash());
	crInitialize();
	crParseArgs(argc, argv);
	struct renderer *renderer = cr_new_renderer();
	if (!crOptionIsSet("is_worker")) {
		size_t bytes = 0;
		char *input = crOptionIsSet("inputFile") ? crReadFile(&bytes) : crReadStdin(&bytes);
		if (!input) {
			crLog("No input provided, exiting.\n");
			cr_destroy_renderer(renderer);
			crDestroyOptions();
			return -1;
		}
		crLog("%zi bytes of input JSON loaded from %s, parsing.\n", bytes, crOptionIsSet("inputFile") ? "file" : "stdin");
		crLoadSceneFromBuf(renderer, input);
		free(input);
		
		crStartRenderer(renderer);
		crWriteImage(renderer);
	} else {
		crStartRenderWorker();
	}
	
	cr_destroy_renderer(renderer);
	crDestroyOptions();
	crLog("Render finished, exiting.\n");
	return 0;
}
