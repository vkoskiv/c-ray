//
//  main.c
//  C-ray
//
//  Created by Valtteri Koskivuori on 12/02/2015.
//  Copyright © 2015-2019 Valtteri Koskivuori. All rights reserved.
//

#define VERSION "0.6"

#include "includes.h"
#include "main.h"

#include "datatypes/camera.h"
#include "utils/logging.h"
#include "utils/filehandler.h"
#include "renderer/renderer.h"
#include "datatypes/scene.h"
#include "utils/ui.h"
#include "utils/learn.h"
#include "utils/multiplatform.h"
#include "datatypes/vertexbuffer.h"
#include "utils/gitsha1.h"

int getFileSize(char *fileName);
void initRenderer(struct renderer *renderer);
int getSysCores(void);

extern struct poly *polygonArray;

/**
 Main entry point

 @param argc Argument count
 @param argv Arguments
 @return Error codes, 0 if exited normally
 */
int main(int argc, char *argv[]) {

	char *hash = gitHash();
	logr(info, "C-ray v%s [%s], Copyright 2015-2019 Valtteri Koskivuori (@vkoskiv)\n", VERSION, hash);
	free(hash);
	
	initTerminal();
	
	allocVertexBuffer();
	//Initialize renderer
	struct renderer *mainRenderer = newRenderer();
	
	//Load the scene and prepare renderer
	if (argc == 2) {
		loadScene(mainRenderer, argv[1], false);
	} else {
		char *scene = readStdin();
		loadScene(mainRenderer, scene, true);
	}
	
	//Initialize SDL display, if available
#ifdef UI_ENABLED
	initSDL(mainRenderer->mainDisplay);
#endif
	
	time_t start, stop;
	
	time(&start);
	render(mainRenderer);
	time(&stop);
	
	printDuration(difftime(stop, start));
	
	//Write to file
	writeImage(mainRenderer);
	
	freeRenderer(mainRenderer);
	freeVertexBuffer();
	
	logr(info, "Render finished, exiting.\n");
	
	return 0;
}
