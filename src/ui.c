//
//  ui.c
//  C-Ray
//
//  Created by Valtteri Koskivuori on 19/02/2017.
//  Copyright © 2017 Valtteri Koskivuori. All rights reserved.
//

#include "ui.h"

display mainDisplay;

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

int initSDL() {
	float windowScale = mainRenderer.worldScene->camera->windowScale;
	
	//Delay so macOS can draw window border (Yeah...)
	for(int i = 0; i < 50; i++){
		SDL_PumpEvents();
		SDL_Delay(1);
	}
	
	//Initialize SDL if need be
	if (mainRenderer.worldScene->camera->showGUI) {
		if (SDL_Init(SDL_INIT_VIDEO) < 0) {
			fprintf(stdout, "SDL couldn't initialize, error %s\n", SDL_GetError());
			return -1;
		}
		//Init window
		mainDisplay.window = SDL_CreateWindow("C-ray © VKoskiv 2015-2017", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, mainRenderer.worldScene->camera->width * windowScale, mainRenderer.worldScene->camera->height * windowScale, SDL_WINDOW_SHOWN | SDL_WINDOW_ALLOW_HIGHDPI);
		if (mainDisplay.window == NULL) {
			fprintf(stdout, "Window couldn't be created, error %s\n", SDL_GetError());
			return -1;
		}
		
		mainDisplay.renderer = SDL_CreateRenderer(mainDisplay.window, -1, SDL_RENDERER_ACCELERATED);
		if (mainDisplay.renderer == NULL) {
			fprintf(stdout, "Renderer couldn't be created, error %s\n", SDL_GetError());
			return -1;
		}
		SDL_RenderSetScale(mainDisplay.renderer, windowScale, windowScale);
		
		mainDisplay.texture = SDL_CreateTexture(mainDisplay.renderer, SDL_PIXELFORMAT_RGB24, SDL_TEXTUREACCESS_STATIC, mainRenderer.worldScene->camera->width, mainRenderer.worldScene->camera->height);
		if (mainDisplay.texture == NULL) {
			fprintf(stdout, "Texture couldn't be created, error %s\n", SDL_GetError());
			return -1;
		}
		
	}
	return 0;
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

void updateProgress(int totalSamples, int completedSamples, int threadNum) {
	printf("Thread %i rendering sample %i/%i\r", threadNum, completedSamples, totalSamples);
	fflush(stdout);
}

void *drawThread(void *arg) {
	SDL_SetRenderDrawColor(mainDisplay.renderer, 0x0, 0x0, 0x0, 0x0);
	SDL_RenderClear(mainDisplay.renderer);
	while (mainRenderer.isRendering) {
		//Check for CTRL-C
		if (signal(SIGINT, sigHandler) == SIG_ERR)
			fprintf(stderr, "Couldn't catch SIGINT\n");
		//Render frame
		pthread_mutex_lock(&mainDisplay.uiMutex);
		SDL_UpdateTexture(mainDisplay.texture, NULL, mainRenderer.worldScene->camera->imgData, mainRenderer.worldScene->camera->width * 3);
		SDL_RenderCopy(mainDisplay.renderer, mainDisplay.texture, NULL, NULL);
		SDL_RenderPresent(mainDisplay.renderer);
		pthread_mutex_unlock(&mainDisplay.uiMutex);
		//Print render status
		/*for (int i = 0; i < mainRenderer.threadCount; i++) {
			updateProgress(mainRenderer.worldScene->camera->sampleCount, mainRenderer.renderThreadInfo[i].completedSamples, mainRenderer.renderThreadInfo[i].thread_num);
		}*/
	}
	mainDisplay.uiThreadInfo->threadComplete = true;
	pthread_exit((void*) arg);
}
