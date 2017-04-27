//
//  main.c
//
//
//  Created by Valtteri Koskivuori on 12/02/15.
//
//

/*
 TODO:
 Full OBJ
 Refraction
 parser
 camera orientation
 */

#include "includes.h"
#include "main.h"

#include "camera.h"
#include "errorhandler.h"
#include "filehandler.h"
#include "renderer.h"
#include "scene.h"
#include "ui.h"

int getFileSize(char *fileName);
int getSysCores();
void freeMem();
void sleepNanosec(int ms);

extern struct renderer mainRenderer;
extern struct poly *polygonArray;

int main(int argc, char *argv[]) {
	
	time_t start, stop;
	
	//Seed RNGs
	srand((int)time(NULL));
#ifndef WINDOWS
	srand48(time(NULL));
#endif
	
	//Initialize renderer
	//FIXME: Put this in a function
	mainRenderer.renderBuffer = NULL;
	mainRenderer.renderTiles = NULL;
	mainRenderer.tileCount = 0;
	mainRenderer.renderedTileCount = 0;
	mainRenderer.activeThreads = 0;
	mainRenderer.threadCount = getSysCores();
	mainRenderer.mode = saveModeNormal;
	mainRenderer.isRendering = false;
	mainRenderer.avgTileTime = (time_t)1;
	mainRenderer.timeSampleCount = 1;
	
	mainRenderer.worldScene = newScene();
	
	char *fileName = NULL;
	//Build the scene
	if (argc == 2) {
		fileName = argv[1];
	} else {
		logHandler(sceneParseErrorNoPath);
	}
	
	//Build the scene
	switch (testBuild(mainRenderer.worldScene, "test")) {
		case -1:
			logHandler(sceneBuildFailed);
			break;
		case -2:
			logHandler(sceneParseErrorMalloc);
			break;
		case 4:
			logHandler(sceneDebugEnabled);
			return 0;
			break;
		default:
			break;
	}
	
	//Check and set threadCount
	int usrThreadCount = mainRenderer.worldScene->camera->threadCount;
	if (usrThreadCount > 0) {
		mainRenderer.threadCount = usrThreadCount;
	}
	
	quantizeImage(mainRenderer.worldScene);
	reorderTiles(mainRenderer.worldScene->camera->tileOrder);
	
#ifdef UI_ENABLED
	mainDisplay.window = NULL;
	mainDisplay.renderer = NULL;
	mainDisplay.texture = NULL;
	mainDisplay.overlayTexture = NULL;
	
	mainDisplay.isBorderless = mainRenderer.worldScene->camera->isBorderless;
	mainDisplay.isFullScreen = mainRenderer.worldScene->camera->isFullScreen;
#endif
	
	//This is a timer to elapse how long a render takes per frame
	time(&start);
	
	//Create threads
	int t;
	
	//Alloc memory for pthread_create() args
	mainRenderer.renderThreadInfo = (struct threadInfo*)calloc(mainRenderer.threadCount, sizeof(struct threadInfo));
	if (mainRenderer.renderThreadInfo == NULL) {
		logHandler(threadMallocFailed);
		return -1;
	}
	
	//Verify sample count
	if (mainRenderer.worldScene->camera->sampleCount < 1) logHandler(renderErrorInvalidSampleCount);
	if (!mainRenderer.worldScene->camera->areaLights) mainRenderer.worldScene->camera->sampleCount = 1;
	
	printf("\nStarting C-ray renderer for frame %i\n\n", mainRenderer.worldScene->camera->currentFrame);
	printf("Rendering at %i x %i\n", mainRenderer.worldScene->camera->width,mainRenderer.worldScene->camera->height);
	printf("Rendering with %i samples\n", mainRenderer.worldScene->camera->sampleCount);
	printf("Rendering with %d thread",mainRenderer.threadCount);
	if (mainRenderer.threadCount > 1) {
		printf("s\n");
	} else {
		printf("\n");
	}
	
	printf("Using %i light bounces\n", mainRenderer.worldScene->camera->bounces);
	printf("Raytracing...\n");
	
	//Allocate memory and create array of pixels for image data
	mainRenderer.worldScene->camera->imgData = (unsigned char*)calloc(3 * mainRenderer.worldScene->camera->width * mainRenderer.worldScene->camera->height, sizeof(unsigned char));
	
	//Allocate memory for render buffer
	//Render buffer is used to store accurate color values for the renderers' internal use
	mainRenderer.renderBuffer = (double*)calloc(3 * mainRenderer.worldScene->camera->width * mainRenderer.worldScene->camera->height, sizeof(double));
	
	//Allocate memory for render UI buffer
	//This buffer is used for storing UI stuff like currently rendering tile highlights
	mainRenderer.uiBuffer = (unsigned char*)calloc(4 * mainRenderer.worldScene->camera->width * mainRenderer.worldScene->camera->height, sizeof(unsigned char));
	
	//Initialize SDL display
#ifdef UI_ENABLED
	initSDL();
#endif
	
	if (!mainRenderer.worldScene->camera->imgData) logHandler(imageMallocFailed);
	
	mainRenderer.isRendering = true;
	mainRenderer.renderAborted = false;
#ifndef WINDOWS
	pthread_attr_init(&mainRenderer.renderThreadAttributes);
	pthread_attr_setdetachstate(&mainRenderer.renderThreadAttributes, PTHREAD_CREATE_JOINABLE);
#endif
	//Main loop (input)
	bool threadsHaveStarted = false;
	while (mainRenderer.isRendering) {
#ifdef UI_ENABLED
		getKeyboardInput();
		drawWindow();
		SDL_UpdateWindowSurface(mainDisplay.window);
#endif
		
		if (!threadsHaveStarted) {
			threadsHaveStarted = true;
			//Create render threads
			for (t = 0; t < mainRenderer.threadCount; t++) {
				mainRenderer.renderThreadInfo[t].thread_num = t;
				mainRenderer.renderThreadInfo[t].threadComplete = false;
				mainRenderer.activeThreads++;
#ifdef WINDOWS
				DWORD threadId;
				mainRenderer.renderThreadInfo[t].thread_handle = CreateThread(NULL, 0, renderThread, &mainRenderer.renderThreadInfo[t], 0, &threadId);
				if (mainRenderer.renderThreadInfo[t].thread_handle == NULL) {
					logHandler(threadCreateFailed);
					exit(-1);
				}
				mainRenderer.renderThreadInfo[t].thread_id = threadId;
#else
				if (pthread_create(&mainRenderer.renderThreadInfo[t].thread_id, &mainRenderer.renderThreadAttributes, renderThread, &mainRenderer.renderThreadInfo[t])) {
					logHandler(threadCreateFailed);
					exit(-1);
				}
#endif
			}
			
			mainRenderer.renderThreadInfo->threadComplete = false;
			
#ifndef WINDOWS
			if (pthread_attr_destroy(&mainRenderer.renderThreadAttributes)) {
				logHandler(threadRemoveFailed);
			}
#endif
		}
		
		//Wait for render threads to finish (Render finished)
		for (t = 0; t < mainRenderer.threadCount; t++) {
			if (mainRenderer.renderThreadInfo[t].threadComplete && mainRenderer.renderThreadInfo[t].thread_num != -1) {
				mainRenderer.activeThreads--;
				mainRenderer.renderThreadInfo[t].thread_num = -1;
			}
			if (mainRenderer.activeThreads == 0 || mainRenderer.renderAborted) {
				mainRenderer.isRendering = false;
			}
		}
		sleepNanosec(16);
	}
	
	//Make sure render threads are finished before continuing
	for (t = 0; t < mainRenderer.threadCount; t++) {
#ifdef WINDOWS
		WaitForSingleObjectEx(mainRenderer.renderThreadInfo[t].thread_handle, INFINITE, FALSE);
#else
		if (pthread_join(mainRenderer.renderThreadInfo[t].thread_id, NULL)) {
			logHandler(threadFrozen);
		}
#endif
	}
	
	time(&stop);
	printDuration(difftime(stop, start));
	
	//Write to file
	
	switch (mainRenderer.mode) {
		case saveModeNormal:
			writeImage(mainRenderer.worldScene->camera->imgData,
					   mainRenderer.worldScene->camera->fileType,
					   mainRenderer.worldScene->camera->currentFrame,
					   mainRenderer.worldScene->camera->width,
					   mainRenderer.worldScene->camera->height);
			break;
		case saveModeNone:
			printf("Image won't be saved!\n");
			break;
		default:
			break;
	}
	
	mainRenderer.worldScene->camera->currentFrame++;
	
	freeMem();
	
	printf("Render finished, exiting.\n");
	
	return 0;
}

void freeMem() {
	//Free memory
	if (mainRenderer.worldScene->camera->imgData)
		free(mainRenderer.worldScene->camera->imgData);
	if (mainRenderer.renderBuffer)
		free(mainRenderer.renderBuffer);
	if (mainRenderer.uiBuffer)
		free(mainRenderer.uiBuffer);
	if (mainRenderer.worldScene->lights)
		free(mainRenderer.worldScene->lights);
	if (mainRenderer.worldScene->spheres)
		free(mainRenderer.worldScene->spheres);
	if (mainRenderer.worldScene->materials)
		free(mainRenderer.worldScene->materials);
	if (vertexArray)
		free(vertexArray);
	if (normalArray)
		free(normalArray);
	if (textureArray)
		free(textureArray);
	if (polygonArray)
		free(polygonArray);
}

void sleepNanosec(int ms) {
#ifdef WINDOWS
	Sleep(ms);
#else
	struct timespec ts;
	ts.tv_sec = ms / 1000;
	ts.tv_nsec = (ms % 1000) * 1000000;
	nanosleep(&ts, NULL);
#endif
}

int getSysCores() {
#ifdef MACOS
	int nm[2];
	size_t len = 4;
	uint32_t count;
	
	nm[0] = CTL_HW; nm[1] = HW_AVAILCPU;
	sysctl(nm, 2, &count, &len, NULL, 0);
	
	if (count < 1) {
		nm[1] = HW_NCPU;
		sysctl(nm, 2, &count, &len, NULL, 0);
		if (count < 1) {
			count = 1;
		}
	}
	return count;
#elif WINDOWS
	SYSTEM_INFO sysInfo;
	GetSystemInfo(&sysInfo);
	return sysInfo.dwNumberOfProcessors;
#else
	return (int)sysconf(_SC_NPROCESSORS_ONLN);
#endif
}

float randRange(float a, float b) {
	return ((b-a)*((float)rand()/RAND_MAX))+a;
}

//FIXME: this may be a duplicate
double rads(double angle) {
	return PIOVER180 * angle;
}
