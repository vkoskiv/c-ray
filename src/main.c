//
//  main.c
//  C-ray
//
//  Created by Valtteri Koskivuori on 12/02/2015.
//  Copyright © 2015-2020 Valtteri Koskivuori. All rights reserved.
//

#include "includes.h"
#include "main.h"

#include "c-ray.h"
#include "utils/logging.h"

int main(int argc, char *argv[]) {
	crInitTerminal();
	char *hash = crGitHash(8);
	logr(info, "C-ray v%s [%s], © 2015-2020 Valtteri Koskivuori\n", crGetVersion(), hash);
	free(hash);
	crInitRenderer();
	char *input;
	size_t bytes = 0;
	input = argc == 2 ? crLoadFile(argv[1], &bytes) : crReadStdin();
	bytes == 0 ?: logr(info, "%zi bytes of input JSON loaded from file, parsing.\n", bytes);
	if (!input) return -1;
	
	if (crLoadSceneFromBuf(input)) {
		crDestroyRenderer();
		return -1;
	}
	crInitSDL();
	crRenderSingleFrame();
	crWriteImage();
	crDestroySDL();
	crDestroyRenderer();
	logr(info, "Render finished, exiting.\n");
	return 0;
}
