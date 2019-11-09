//
//  main.c
//  C-ray
//
//  Created by Valtteri Koskivuori on 12/02/2015.
//  Copyright © 2015-2019 Valtteri Koskivuori. All rights reserved.
//

#define VERSION "0.6.1"

#include "includes.h"
#include "main.h"

#include "utils/logging.h"
#include "utils/filehandler.h"
#include "renderer/renderer.h"
#include "datatypes/scene.h"
#include "utils/ui.h"
#include "utils/multiplatform.h"
#include "datatypes/vertexbuffer.h"
#include "utils/gitsha1.h"
#include "datatypes/texture.h"

/**
 Main entry point

 @param argc Argument count
 @param argv Arguments
 @return Error codes, 0 if exited normally
 */
int main(int argc, char *argv[]) {
	char *hash = gitHash(8);
	logr(info, "C-ray v%s [%s], © 2015-2019 Valtteri Koskivuori\n", VERSION, hash);
	
	initTerminal();
	allocVertexBuffer();
	struct renderer *r = newRenderer();
	
	if (loadScene(r, argc, argv)) {
		freeRenderer(r);
		freeVertexBuffer();
		return -1;
	}
	
#ifdef UI_ENABLED
	initSDL(r->mainDisplay);
#endif
	
	time_t start, stop;
	time(&start);
	render(r);
	time(&stop);
	printDuration(difftime(stop, start));
	
	writeImage(r->state.image, r->prefs.fileMode, (struct renderInfo){
		.bounces = r->prefs.bounces,
		.samples = r->prefs.sampleCount,
		.crayVersion = VERSION,
		.gitHash = hash,
		.renderTimeSeconds = difftime(stop, start)
	});
	free(hash);
	freeRenderer(r);
	freeVertexBuffer();
	logr(info, "Render finished, exiting.\n");
	return 0;
}
