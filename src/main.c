//
//  main.c
//  
//
//  Created by Valtteri Koskivuori on 12/02/15.
//
//

#include "main.h"

int getFileSize(char *fileName);
int getSysCores();

int main(int argc, char *argv[]) {
	
	time_t start, stop;
	
	//Seed RNGs
	srand((int)time(NULL));
	srand48(time(NULL));
	
	//Initialize renderer
	//FIXME: Put this in a function
	mainRenderer.worldScene = NULL;
	mainRenderer.renderBuffer = NULL;
	mainRenderer.renderTiles = NULL;
	mainRenderer.tileCount = 0;
	mainRenderer.renderedTileCount = 0;
	mainRenderer.activeThreads = 0;
	mainRenderer.threadCount = 16;//getSysCores();
	mainRenderer.shouldSave = true;
	mainRenderer.isRendering = false;
	
	//Prepare the scene
	world sceneObject;
	sceneObject.materials = NULL;
	sceneObject.spheres = NULL;
	sceneObject.lights = NULL;
	sceneObject.objs = NULL;
	mainRenderer.worldScene = &sceneObject; //Assign to global variable
	
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
	
	quantizeImage(mainRenderer.worldScene);
	
	//Initialize SDL display
	initSDL();
	
	//This is a timer to elapse how long a render takes per frame
	time(&start);
	
	if (mainRenderer.worldScene->camera->forceSingleCore) mainRenderer.threadCount = 1;
	
	//Create threads
	int t;
	
	//Alloc memory for pthread_create() args
	mainRenderer.renderThreadInfo = calloc(mainRenderer.threadCount, sizeof(threadInfo));
	if (mainRenderer.renderThreadInfo == NULL) {
		logHandler(threadMallocFailed);
		return -1;
	}
	
	//Alloc memory for uiThread args
	mainDisplay.uiThreadInfo = calloc(1, sizeof(threadInfo));
	if (mainDisplay.uiThreadInfo == NULL) {
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
		printf("s");
	}
	if (mainRenderer.worldScene->camera->forceSingleCore) printf(" (Forced single thread)\n");
	else printf("\n");
	
	printf("Using %i light bounces\n", mainRenderer.worldScene->camera->bounces);
	printf("Raytracing...\n");
	
	//Allocate memory and create array of pixels for image data
	mainRenderer.worldScene->camera->imgData = (unsigned char*)calloc(4 * mainRenderer.worldScene->camera->width * mainRenderer.worldScene->camera->height, sizeof(unsigned char));
	
	//Allocate memory for render buffer
	//Render buffer is used to store accurate color values for the renderers' internal use
	mainRenderer.renderBuffer = (double*)calloc(4 * mainRenderer.worldScene->camera->width * mainRenderer.worldScene->camera->height, sizeof(double));
	
	if (!mainRenderer.worldScene->camera->imgData) logHandler(imageMallocFailed);
	
	mainRenderer.isRendering = true;
	pthread_attr_init(&mainRenderer.renderThreadAttributes);
	pthread_attr_init(&mainDisplay.uiThreadAttributes);
	pthread_attr_setdetachstate(&mainRenderer.renderThreadAttributes, PTHREAD_CREATE_JOINABLE);
	pthread_attr_setdetachstate(&mainDisplay.uiThreadAttributes, PTHREAD_CREATE_JOINABLE);
	
	//Main loop (input)
	bool threadsHaveStarted = false;
	while (mainRenderer.isRendering) {
		getKeyboardInput();
		
		if (!threadsHaveStarted) {
			threadsHaveStarted = true;
			//Create render threads
			for (t = 0; t < mainRenderer.threadCount; t++) {
				mainRenderer.renderThreadInfo[t].thread_num = t;
				mainRenderer.renderThreadInfo[t].threadComplete = false;
				mainRenderer.activeThreads++;
				if (pthread_create(&mainRenderer.renderThreadInfo[t].thread_id, &mainRenderer.renderThreadAttributes, renderThread, &mainRenderer.renderThreadInfo[t])) {
					logHandler(threadCreateFailed);
					exit(-1);
				}
			}
			
			mainRenderer.renderThreadInfo->threadComplete = false;
			//Create UI render thread
			if (pthread_create(&mainDisplay.uiThreadInfo->thread_id, &mainDisplay.uiThreadAttributes, drawThread, mainDisplay.uiThreadInfo)) {
				logHandler(threadCreateFailed);
				exit(-1);
			}
			
			if (pthread_attr_destroy(&mainRenderer.renderThreadAttributes)) {
				logHandler(threadRemoveFailed);
			}
		}
		
		//Wait for render threads to finish (Render finished)
		for (t = 0; t < mainRenderer.threadCount; t++) {
			if (mainRenderer.renderThreadInfo[t].threadComplete && mainRenderer.renderThreadInfo[t].thread_num != -1) {
				mainRenderer.activeThreads--;
				mainRenderer.renderThreadInfo[t].thread_num = -1;
			}
			if (mainRenderer.activeThreads == 0) {
				mainRenderer.isRendering = false;
			}
		}
		
		//Wait for UI render thread to finish
		if (mainDisplay.uiThreadInfo->threadComplete) {
			mainRenderer.isRendering = false;
		}
	}
	
	time(&stop);
	printDuration(difftime(stop, start));
	
	//Write to file
	if (mainRenderer.shouldSave)
		writeImage(mainRenderer.worldScene);
	else
		printf("Image won't be saved!\n");
	
	mainRenderer.worldScene->camera->currentFrame++;
	
	//Free memory
	if (mainRenderer.worldScene->camera->imgData)
		free(mainRenderer.worldScene->camera->imgData);
	if (mainRenderer.worldScene->lights)
		free(mainRenderer.worldScene->lights);
	if (mainRenderer.worldScene->spheres)
		free(mainRenderer.worldScene->spheres);
	if (mainRenderer.worldScene->materials)
		free(mainRenderer.worldScene->materials);
	
	printf("Render finished, exiting.\n");
	
	return 0;
}

#pragma mark Helper funcs

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
#elif WIN32
	SYSTEM_INFO sysInfo;
	GetSystemInfo(&sysInfo);
	return sysinfo.dwNumberOfProcessors;
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
