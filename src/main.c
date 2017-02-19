//
//  main.c
//  
//
//  Created by Valtteri Koskivuori on 12/02/15.
//
//

#include "main.h"

//SDL globals
SDL_Window *window = NULL;
SDL_Renderer *windowRenderer = NULL;
SDL_Texture *texture = NULL;
pthread_mutex_t uimutex = PTHREAD_MUTEX_INITIALIZER;

//Thread globals
threadInfo *tinfo;
threadInfo *uitinfo;
pthread_attr_t attributes;
pthread_attr_t uiattributes;

//Function prototypes
void *drawThread(void *arg);
//void updateProgress(int y, int max, int min);
void updateProgress(int totalSamples, int completedSamples, int threadNum);
void printDuration(double time);
int getFileSize(char *fileName);
int getSysCores();
void getKeyboardInput();

//Signal handling
void (*signal(int signo, void (*func )(int)))(int);
typedef void sigfunc(int);
sigfunc *signal(int, sigfunc*);

void sigHandler(int sig) {
	if (sig == SIGINT) {
		printf("Received CTRL-C, aborting...\n");
		exit(1);
	}
}

int main(int argc, char *argv[]) {
	
	time_t start, stop;
	
	//Seed RNGs
	srand((int)time(NULL));
	srand48(time(NULL));
	
	//Initialize renderer
	mainRenderer.worldScene = NULL;
	mainRenderer.renderBuffer = NULL;
	mainRenderer.activeThreads = 0;
	mainRenderer.sectionSize = 0;
	mainRenderer.threadCount = getSysCores();
	mainRenderer.shouldSave = true;
	mainRenderer.isRendering = false;
	
	//Prepare the scene
	world sceneObject;
	sceneObject.materials = NULL;
	sceneObject.spheres = NULL;
	sceneObject.lights = NULL;
	mainRenderer.worldScene = &sceneObject; //Assign to global variable
	
	int frame = 0;
	
	//Animation
	do {
		//This is a timer to elapse how long a render takes per frame
		time(&start);
		
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
		
		float windowScale = mainRenderer.worldScene->camera->windowScale;
		
		//Initialize SDL if need be
		if (mainRenderer.worldScene->camera->showGUI) {
			if (SDL_Init(SDL_INIT_VIDEO) < 0) {
				fprintf(stdout, "SDL couldn't initialize, error %s\n", SDL_GetError());
				return -1;
			}
			//Init window
			window = SDL_CreateWindow("C-ray © VKoskiv 2015-2017", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, mainRenderer.worldScene->camera->width * windowScale, mainRenderer.worldScene->camera->height * windowScale, SDL_WINDOW_SHOWN | SDL_WINDOW_ALLOW_HIGHDPI);
			if (window == NULL) {
				fprintf(stdout, "Window couldn't be created, error %s\n", SDL_GetError());
				return -1;
			}
			
			windowRenderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
			if (windowRenderer == NULL) {
				fprintf(stdout, "Renderer couldn't be created, error %s\n", SDL_GetError());
				return -1;
			}
			SDL_RenderSetScale(windowRenderer, windowScale, windowScale);
			
			texture = SDL_CreateTexture(windowRenderer, SDL_PIXELFORMAT_RGB24, SDL_TEXTUREACCESS_STATIC, mainRenderer.worldScene->camera->width, mainRenderer.worldScene->camera->height);
			if (texture == NULL) {
				fprintf(stdout, "Texture couldn't be created, error %s\n", SDL_GetError());
				return -1;
			}
			
		}
		
		//Delay so macOS can draw window border (Yeah...)
		for(int i = 0; i < 50; i++){
			SDL_PumpEvents();
			SDL_Delay(1);
		}
		
		if (mainRenderer.worldScene->camera->forceSingleCore) mainRenderer.threadCount = 1;
		
		//Create threads
		int t;
		
		//Alloc memory for pthread_create() args
		tinfo = calloc(mainRenderer.threadCount, sizeof(threadInfo));
		if (tinfo == NULL) {
			logHandler(threadMallocFailed);
			return -1;
		}
		
		//Alloc memory for uiThread args
		uitinfo = calloc(1, sizeof(threadInfo));
		if (uitinfo == NULL) {
			logHandler(threadMallocFailed);
			return -1;
		}
		
		//Verify sample count
		if (mainRenderer.worldScene->camera->sampleCount < 1) logHandler(renderErrorInvalidSampleCount);
		if (!mainRenderer.worldScene->camera->areaLights) mainRenderer.worldScene->camera->sampleCount = 1;
		
		mainRenderer.worldScene->camera->currentFrame = frame;
		frame++;
		
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
		
		//Calculate section sizes for every thread, multiple threads can't render the same portion of an image
		mainRenderer.sectionSize = mainRenderer.worldScene->camera->height / mainRenderer.threadCount;
		if ((mainRenderer.sectionSize % 2) != 0) logHandler(invalidThreadCount);
		mainRenderer.isRendering = true;
		pthread_attr_init(&attributes);
		pthread_attr_init(&uiattributes);
		pthread_attr_setdetachstate(&attributes, PTHREAD_CREATE_JOINABLE);
		pthread_attr_setdetachstate(&uiattributes, PTHREAD_CREATE_JOINABLE);
		
		//Main loop (input)
		bool threadsHaveStarted = false;
		while (mainRenderer.isRendering) {
			getKeyboardInput();
			
			if (!threadsHaveStarted) {
				threadsHaveStarted = true;
				//Create render threads
				for (t = 0; t < mainRenderer.threadCount; t++) {
					tinfo[t].thread_num = t;
					tinfo[t].threadComplete = false;
					mainRenderer.activeThreads++;
					if (pthread_create(&tinfo[t].thread_id, &attributes, renderThread, &tinfo[t])) {
						logHandler(threadCreateFailed);
						exit(-1);
					}
				}
				
				uitinfo->threadComplete = false;
				//Create UI render thread
				if (pthread_create(&uitinfo->thread_id, &uiattributes, drawThread, uitinfo)) {
					logHandler(threadCreateFailed);
					exit(-1);
				}
				
				if (pthread_attr_destroy(&attributes)) {
					logHandler(threadRemoveFailed);
				}
			}
			
			//Wait for render threads to finish (Render finished)
			for (t = 0; t < mainRenderer.threadCount; t++) {
				if (tinfo[t].threadComplete && tinfo[t].thread_num != -1) {
					mainRenderer.activeThreads--;
					tinfo[t].thread_num = -1;
				}
				if (mainRenderer.activeThreads == 0) {
					mainRenderer.isRendering = false;
				}
			}
			
			//Wait for UI render thread to finish
			if (uitinfo->threadComplete) {
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
	} while (mainRenderer.worldScene->camera->currentFrame < mainRenderer.worldScene->camera->frameCount);
	
	printf("Render finished, exiting.\n");
	
	return 0;
}

#pragma mark UI

void *drawThread(void *arg) {
	SDL_SetRenderDrawColor(windowRenderer, 0x0, 0x0, 0x0, 0x0);
	SDL_RenderClear(windowRenderer);
	while (mainRenderer.isRendering) {
		//Check for CTRL-C
		if (signal(SIGINT, sigHandler) == SIG_ERR)
			fprintf(stderr, "Couldn't catch SIGINT\n");
		//Render frame
		pthread_mutex_lock(&uimutex);
		SDL_UpdateTexture(texture, NULL, mainRenderer.worldScene->camera->imgData, mainRenderer.worldScene->camera->width * 3);
		SDL_RenderCopy(windowRenderer, texture, NULL, NULL);
		SDL_RenderPresent(windowRenderer);
		pthread_mutex_unlock(&uimutex);
		//Print render status
		for (int i = 0; i < mainRenderer.threadCount; i++) {
			updateProgress(mainRenderer.worldScene->camera->sampleCount, tinfo[i].completedSamples, tinfo[i].thread_num);
		}
	}
	uitinfo->threadComplete = true;
	pthread_exit((void*) arg);
}

void updateProgress(int totalSamples, int completedSamples, int threadNum) {
	printf("Thread %i rendering sample %i/%i\r", threadNum, completedSamples, totalSamples);
	fflush(stdout);
}

void printDuration(double time) {
	if (time <= 60) {
		printf("Finished render in %.0f seconds.\n", time);
	} else if (time <= 3600) {
		printf("Finished render in %.0f minute", time/60);
		if (time/60 > 1) printf("s.\n"); else printf(".\n");
	} else {
		printf("Finished render in %.0f hours (%.0f min).\n", (time/60)/60, time/60);
	}
}

void getKeyboardInput() {
	SDL_PumpEvents();
	const Uint8 *keys = SDL_GetKeyboardState(NULL);
	if (keys[SDL_SCANCODE_S]) {
		printf("Aborting render, saving...\n");
		mainRenderer.isRendering = false;
	}
	if (keys[SDL_SCANCODE_X]) {
		printf("Aborting render without saving...\n");
		mainRenderer.shouldSave = false;
		mainRenderer.isRendering = false;
	}
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

double rads(double angle) {
    return PIOVER180 * angle;
}
