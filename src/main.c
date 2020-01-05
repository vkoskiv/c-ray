//
//  main.c
//  C-ray
//
//  Created by Valtteri Koskivuori on 12/02/2015.
//  Copyright © 2015-2020 Valtteri Koskivuori. All rights reserved.
//

#define VERSION "0.6.2"

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
#include "utils/hashtable.h"

int main(int argc, char *argv[]) {
	char *hash = gitHash(8);
	logr(info, "C-ray v%s [%s], © 2015-2020 Valtteri Koskivuori\n", VERSION, hash);
	initTerminal();
	allocVertexBuffer();
	struct renderer *r = newRenderer();
	
	if (loadScene(r, argc, argv)) {
		free(hash);
		freeVertexBuffer();
		freeRenderer(r);
		return -1;
	}
	
	initSDL(r->mainDisplay);
	
	time_t start, stop;
	time(&start);
	struct texture *output = renderFrame(r);
	time(&stop);
	printDuration(difftime(stop, start));
	
	writeImage(output, r->prefs.fileMode, (struct renderInfo){
		.bounces = r->prefs.bounces,
		.samples = r->prefs.sampleCount,
		.crayVersion = VERSION,
		.gitHash = hash,
		.renderTimeSeconds = difftime(stop, start),
		.threadCount = r->prefs.threadCount
	});
	freeTexture(output);
	free(hash);
	freeRenderer(r);
	freeVertexBuffer();
	logr(info, "Render finished, exiting.\n");
	return 0;
}
