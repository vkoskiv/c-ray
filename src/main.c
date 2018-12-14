//
//  main.c
//  C-ray
//
//  Created by Valtteri Koskivuori on 12/02/2015.
//  Copyright © 2015-2018 Valtteri Koskivuori. All rights reserved.
//

#include "includes.h"
#include "main.h"

#include "camera.h"
#include "logging.h"
#include "filehandler.h"
#include "renderer.h"
#include "scene.h"
#include "ui.h"

int getFileSize(char *fileName);
void initRenderer(struct renderer *renderer);
int getSysCores(void);
void freeGlobals(void);
void prepareGlobals(void);

extern struct poly *polygonArray;

/**
 Main entry point

 @param argc Argument count
 @param argv Arguments
 @return Error codes, 0 if exited normally
 */
int main(int argc, char *argv[]) {
	//Seed RNGs
	srand((int)time(NULL));
#ifndef WINDOWS
	srand48(time(NULL));
#endif
	
	initTerminal();
	
	prepareGlobals();
	//Initialize renderer
	struct renderer *mainRenderer = newRenderer();
	
	//Load the scene and prepare renderer
	if (argc == 2) {
		loadScene(mainRenderer, argv[1]);
	} else {
		logr(error, "Invalid input file path.\n");
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
	freeGlobals();
	
	logr(info, "Render finished, exiting.\n");
	
	return 0;
}

void freeGlobals() {
	//Free memory
	if (vertexArray)
		free(vertexArray);
	if (normalArray)
		free(normalArray);
	if (textureArray)
		free(textureArray);
	if (polygonArray)
		free(polygonArray);
}

void prepareGlobals() {
	vertexArray = calloc(1, sizeof(struct vector));
	normalArray = calloc(1, sizeof(struct vector));
	textureArray = calloc(1, sizeof(struct coord));
	polygonArray = calloc(1, sizeof(struct poly));
	
	vertexCount = 0;
	normalCount = 0;
	textureCount = 0;
	polyCount = 0;
}
