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
	crParseOptions(argc, argv);
	crInitTerminal();
	logr(info, "C-ray v%s [%.8s], © 2015-2020 Valtteri Koskivuori\n", crGetVersion(), crGitHash());
	crInitRenderer();
	size_t bytes = 0;
	char *input = crOptionIsSet("inputFile") ? crLoadFile(crPathArg(), &bytes) : crReadStdin(&bytes);
	crSetAssetPath(crOptionIsSet("inputFile") ? crGetFilePath(crPathArg()) : "./");
	if (input) logr(info, "%zi bytes of input JSON loaded from %s, parsing.\n", bytes, crOptionIsSet("inputFile") ? "file" : "stdin");
	if (!input || crLoadSceneFromBuf(input)) {
		if (input) free(input);
		crDestroyRenderer();
		crRestoreTerminal();
		return -1;
	}
	free(input);
	crRenderSingleFrame();
	crWriteImage();
	crDestroyRenderer();
	crRestoreTerminal();
	crDestroyOptions();
	logr(info, "Render finished, exiting.\n");
	return 0;
}
