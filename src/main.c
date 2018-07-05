//
//  main.c
//
//
//  Created by Valtteri Koskivuori on 12/02/15.
//
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
void freeMem(struct renderer *renderer);

extern struct poly *polygonArray;

/**
 Main entry point

 @param argc Argument count
 @param argv Arguments
 @return Error codes, 0 if exited normally
 */
int main(int argc, char *argv[]) {
	
	time_t start, stop;
	
	//Seed RNGs
	srand((int)time(NULL));
#ifndef WINDOWS
	srand48(time(NULL));
#endif
	
	//Disable output buffering
	setbuf(stdout, NULL);
	
	struct renderer *mainRenderer = (struct renderer*)calloc(1, sizeof(struct renderer));
	
	//Initialize renderer
	initRenderer(mainRenderer);
	
	char *fileName = NULL;
	//Build the scene
	if (argc == 2) {
		fileName = argv[1];
	} else {
		logr(error, "Invalid input file path.\n");
	}
	
#ifndef UI_ENABLED
	printf("**************************************************************************\n");
	printf("*      UI is DISABLED! Enable by installing SDL2 and doing `cmake .`     *\n");
	printf("**************************************************************************\n");
#endif
	
#ifdef UI_ENABLED
	mainRenderer->mainDisplay = (struct display*)calloc(1, sizeof(struct display));
	mainRenderer->mainDisplay->window = NULL;
	mainRenderer->mainDisplay->renderer = NULL;
	mainRenderer->mainDisplay->texture = NULL;
	mainRenderer->mainDisplay->overlayTexture = NULL;
#endif
	
	//Build the scene
	switch (parseJSON(mainRenderer, fileName)) {
		case -1:
			logr(error, "Scene builder failed due to previous error.");
			break;
		case 4:
			logr(warning, "Scene debug mode enabled, won't render image.");
			return 0;
			break;
		case -2:
			logr(error, "JSON parser failed.");
			break;
		default:
			break;
	}
	
	mainRenderer->threadPaused = (bool*)calloc(mainRenderer->threadCount, sizeof(bool));
	
	//Quantize image into renderTiles
	quantizeImage(mainRenderer);
	//Compute the focal length for the camera
	computeFocalLength(mainRenderer);
	
	//This is a timer to elapse how long a render takes per frame
	time(&start);
	
	//Create threads
	int t;
	
	//Alloc memory for pthread_create() args
	mainRenderer->renderThreadInfo = (struct threadInfo*)calloc(mainRenderer->threadCount, sizeof(struct threadInfo));
	if (mainRenderer->renderThreadInfo == NULL) {
		logr(error, "Failed to allocate memory for threadInfo args.\n");
	}
	
	//Verify sample count
	if (mainRenderer->sampleCount < 1) {
		logr(warning, "Invalid sample count given, setting to 1\n");
		mainRenderer->sampleCount = 1;
	}
	
	logr(info, "Starting C-ray renderer for frame %i\n", mainRenderer->currentFrame);
	
	//Print a useful warning to user if the defined tile size results in less renderThreads
	if (mainRenderer->tileCount < mainRenderer->threadCount) {
		logr(warning, "WARNING: Rendering with a less than optimal thread count due to large tile size!\n");
		logr(warning, "Reducing thread count from %i to ", mainRenderer->threadCount);
		mainRenderer->threadCount = mainRenderer->tileCount;
		printf("%i\n", mainRenderer->threadCount);
	}
	
	logr(info, "Rendering at %i x %i\n", mainRenderer->image->size.width,mainRenderer->image->size.height);
	logr(info, "Rendering %i samples with %i bounces.\n", mainRenderer->sampleCount, mainRenderer->scene->bounces);
	logr(info, "Rendering with %d thread", mainRenderer->threadCount);
	if (mainRenderer->threadCount > 1) {
		printf("s.\n");
	} else {
		printf(".\n");
	}
	
	logr(info, "Pathtracing...\n");
	
	//Allocate memory and create array of pixels for image data
	mainRenderer->image->data = (unsigned char*)calloc(3 * mainRenderer->image->size.width * mainRenderer->image->size.height, sizeof(unsigned char));
	if (!mainRenderer->image->data) {
		logr(error, "Failed to allocate memory for image data.");
	}
	//Allocate memory for render buffer
	//Render buffer is used to store accurate color values for the renderers' internal use
	mainRenderer->renderBuffer = (double*)calloc(3 * mainRenderer->image->size.width * mainRenderer->image->size.height, sizeof(double));
	
	//Allocate memory for render UI buffer
	//This buffer is used for storing UI stuff like currently rendering tile highlights
	mainRenderer->uiBuffer = (unsigned char*)calloc(4 * mainRenderer->image->size.width * mainRenderer->image->size.height, sizeof(unsigned char));
	
	//Initialize SDL display
#ifdef UI_ENABLED
	initSDL(mainRenderer);
#endif
	
	mainRenderer->isRendering = true;
	mainRenderer->renderAborted = false;
#ifndef WINDOWS
	pthread_attr_init(&mainRenderer->renderThreadAttributes);
	pthread_attr_setdetachstate(&mainRenderer->renderThreadAttributes, PTHREAD_CREATE_JOINABLE);
#endif
	//Main loop (input)
	bool threadsHaveStarted = false;
	while (mainRenderer->isRendering) {
#ifdef UI_ENABLED
		getKeyboardInput(mainRenderer);
		drawWindow(mainRenderer);
		SDL_UpdateWindowSurface(mainRenderer->mainDisplay->window);
#endif
		
		if (!threadsHaveStarted) {
			threadsHaveStarted = true;
			//Create render threads
			for (t = 0; t < mainRenderer->threadCount; t++) {
				mainRenderer->renderThreadInfo[t].thread_num = t;
				mainRenderer->renderThreadInfo[t].threadComplete = false;
				mainRenderer->renderThreadInfo[t].r = mainRenderer;
				mainRenderer->activeThreads++;
#ifdef WINDOWS
				DWORD threadId;
				mainRenderer->renderThreadInfo[t].thread_handle = CreateThread(NULL, 0, renderThread, &mainRenderer->renderThreadInfo[t], 0, &threadId);
				if (mainRenderer->renderThreadInfo[t].thread_handle == NULL) {
					logr(error, "Failed to create thread.\n");
					exit(-1);
				}
				mainRenderer->renderThreadInfo[t].thread_id = threadId;
#else
				if (pthread_create(&mainRenderer->renderThreadInfo[t].thread_id, &mainRenderer->renderThreadAttributes, renderThread, &mainRenderer->renderThreadInfo[t])) {
					logr(error, "Failed to create a thread.\n");
				}
#endif
			}
			
			mainRenderer->renderThreadInfo->threadComplete = false;
			
#ifndef WINDOWS
			if (pthread_attr_destroy(&mainRenderer->renderThreadAttributes)) {
				logr(warning, "Failed to destroy pthread.\n");
			}
#endif
		}
		
		//Wait for render threads to finish (Render finished)
		for (t = 0; t < mainRenderer->threadCount; t++) {
			if (mainRenderer->renderThreadInfo[t].threadComplete && mainRenderer->renderThreadInfo[t].thread_num != -1) {
				mainRenderer->activeThreads--;
				mainRenderer->renderThreadInfo[t].thread_num = -1;
			}
			if (mainRenderer->activeThreads == 0 || mainRenderer->renderAborted) {
				mainRenderer->isRendering = false;
			}
		}
		sleepMSec(100);
	}
	
	//Make sure render threads are finished before continuing
	for (t = 0; t < mainRenderer->threadCount; t++) {
#ifdef WINDOWS
		WaitForSingleObjectEx(mainRenderer->renderThreadInfo[t].thread_handle, INFINITE, FALSE);
#else
		if (pthread_join(mainRenderer->renderThreadInfo[t].thread_id, NULL)) {
			logr(warning, "Thread %t frozen.", t);
		}
#endif
	}
	
	time(&stop);
	printDuration(difftime(stop, start));
	
	//Write to file
	writeImage(mainRenderer);
	
	mainRenderer->currentFrame++;
	
	freeMem(mainRenderer);
	
	logr(info, "Render finished, exiting.\n");
	
	return 0;
}


/**
 Free dynamically allocated memory
 */
void freeMem(struct renderer *renderer) {
	//Free memory
	if (renderer->image->data)
		free(renderer->image->data);
	if (renderer->renderThreadInfo)
		free(renderer->renderThreadInfo);
	if (renderer->renderBuffer)
		free(renderer->renderBuffer);
	if (renderer->uiBuffer)
		free(renderer->uiBuffer);
	if (renderer->scene->lights)
		free(renderer->scene->lights);
	if (renderer->scene->spheres)
		free(renderer->scene->spheres);
	if (renderer->scene->materials)
		free(renderer->scene->materials);
	if (renderer->renderTiles)
		free(renderer->renderTiles);
	if (renderer->scene)
		free(renderer->scene);
	if (vertexArray)
		free(vertexArray);
	if (normalArray)
		free(normalArray);
	if (textureArray)
		free(textureArray);
	if (polygonArray)
		free(polygonArray);
}

/**
 Sleep for a given amount of milliseconds

 @param ms Milliseconds to sleep for
 */
void sleepMSec(int ms) {
#ifdef _WIN32
	Sleep(ms);
#elif __APPLE__
	struct timespec ts;
	ts.tv_sec = ms / 1000;
	ts.tv_nsec = (ms % 1000) * 1000000;
	nanosleep(&ts, NULL);
#elif __linux__
	usleep(ms * 1000);
#endif
}
