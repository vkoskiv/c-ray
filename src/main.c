//
//  main.c
//  C-ray
//
//  Created by Valtteri Koskivuori on 12/02/2015.
//  Copyright © 2015-2020 Valtteri Koskivuori. All rights reserved.
//

#include <stdlib.h>
#include <stdbool.h>
#include "main.h"

#include "c-ray.h"
#include "utils/logging.h"

int main(int argc, char *argv[]) {
	crInitTerminal();
	char *hash = crGitHash(8);
	logr(info, "C-ray v%s [%s], © 2015-2020 Valtteri Koskivuori\n", crGetVersion(), hash);
	free(hash);
	crInitRenderer();
	size_t bytes = 0;
	char *input = argc == 2 ? crLoadFile(argv[1], &bytes) : crReadStdin(&bytes);
	logr(info, "%zi bytes of input JSON loaded from %s, parsing.\n", bytes, argc == 2 ? "file" : "stdin");
	if (!input) return -1;
	if (crLoadSceneFromBuf(input)) {
		free(input);
		crDestroyRenderer();
		return -1;
	}
	free(input);
	crInitSDL();
	crRenderSingleFrame();
	crWriteImage();
	crDestroySDL();
	crDestroyRenderer();
	logr(info, "Render finished, exiting.\n");
	return 0;
}
