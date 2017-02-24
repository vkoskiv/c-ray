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

void fillTexture(SDL_Renderer *renderer, SDL_Texture *texture, int r, int g, int b, int a) {
	SDL_SetRenderTarget(renderer, texture);
	SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
	SDL_SetRenderDrawColor(renderer, r, g, b, a);
	SDL_RenderFillRect(renderer, NULL);
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
		//Init renderer
		mainDisplay.renderer = SDL_CreateRenderer(mainDisplay.window, -1, SDL_RENDERER_ACCELERATED);
		if (mainDisplay.renderer == NULL) {
			fprintf(stdout, "Renderer couldn't be created, error %s\n", SDL_GetError());
			return -1;
		}
		//And set blend modes
		SDL_SetRenderDrawBlendMode(mainDisplay.renderer, SDL_BLENDMODE_BLEND);
		
		SDL_RenderSetScale(mainDisplay.renderer, windowScale, windowScale);
		//Init pixel texture
		mainDisplay.texture = SDL_CreateTexture(mainDisplay.renderer, SDL_PIXELFORMAT_RGB24, SDL_TEXTUREACCESS_STREAMING, mainRenderer.worldScene->camera->width, mainRenderer.worldScene->camera->height);
		if (mainDisplay.texture == NULL) {
			fprintf(stdout, "Texture couldn't be created, error %s\n", SDL_GetError());
			return -1;
		}
		//Init overlay texture (for UI info)
		mainDisplay.overlayTexture = SDL_CreateTexture(mainDisplay.renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STREAMING, mainRenderer.worldScene->camera->width, mainRenderer.worldScene->camera->height);
		if (mainDisplay.overlayTexture == NULL) {
			fprintf(stdout, "Overlay texture couldn't be created, error %s\n", SDL_GetError());
			return -1;
		}
		
		//And set blend modes for textures too
		SDL_SetTextureBlendMode(mainDisplay.texture, SDL_BLENDMODE_BLEND);
		SDL_SetTextureBlendMode(mainDisplay.overlayTexture, SDL_BLENDMODE_BLEND);
	}
	return 0;
}

void getKeyboardInput() {
	SDL_PumpEvents();
	const Uint8 *keys = SDL_GetKeyboardState(NULL);
	if (keys[SDL_SCANCODE_S]) {
		printf("Aborting render, saving...\n");
		mainRenderer.renderAborted = true;
	}
	if (keys[SDL_SCANCODE_X]) {
		printf("Aborting render without saving...\n");
		mainRenderer.shouldSave = false;
		mainRenderer.renderAborted = true;
	}
}

void updateProgress(int totalSamples, int completedSamples, int threadNum) {
	printf("Thread %i rendering sample %i/%i\r", threadNum, completedSamples, totalSamples);
	fflush(stdout);
}

void drawPixel(int x, int y, bool on) {
	if (y < 0) y = 1;
	if (x < 0) x = 1;
	
	if (on) {
		mainRenderer.uiBuffer[(x + (mainRenderer.worldScene->camera->height - y)
							   * mainRenderer.worldScene->camera->width) * 4 + 3] = (unsigned char)min(frameColor.red*255.0f, 255.0f);
		mainRenderer.uiBuffer[(x + (mainRenderer.worldScene->camera->height - y)
							   * mainRenderer.worldScene->camera->width) * 4 + 2] = (unsigned char)min(frameColor.green*255.0f, 255.0f);
		mainRenderer.uiBuffer[(x + (mainRenderer.worldScene->camera->height - y)
							   * mainRenderer.worldScene->camera->width) * 4 + 1] = (unsigned char)min(frameColor.blue*255.0f, 255.0f);
		mainRenderer.uiBuffer[(x + (mainRenderer.worldScene->camera->height - y)
							   * mainRenderer.worldScene->camera->width) * 4 + 0] = (unsigned char)min(255.0f, 255.0f);
	} else {
		mainRenderer.uiBuffer[(x + (mainRenderer.worldScene->camera->height - y) * mainRenderer.worldScene->camera->width) * 4 + 0] = (unsigned char)0;
		mainRenderer.uiBuffer[(x + (mainRenderer.worldScene->camera->height - y) * mainRenderer.worldScene->camera->width) * 4 + 1] = (unsigned char)0;
		mainRenderer.uiBuffer[(x + (mainRenderer.worldScene->camera->height - y) * mainRenderer.worldScene->camera->width) * 4 + 2] = (unsigned char)0;
		mainRenderer.uiBuffer[(x + (mainRenderer.worldScene->camera->height - y) * mainRenderer.worldScene->camera->width) * 4 + 3] = (unsigned char)0;
	}
}

void drawFrame(renderTile tile, bool on) {
	//top left
	drawPixel(tile.startX+1, tile.startY+1, on);
	drawPixel(tile.startX+2, tile.startY+1, on);
	drawPixel(tile.startX+3, tile.startY+1, on);
	drawPixel(tile.startX+4, tile.startY+1, on);
	drawPixel(tile.startX+5, tile.startY+1, on);
	drawPixel(tile.startX+6, tile.startY+1, on);
	
	drawPixel(tile.startX+1, tile.startY+1, on);
	drawPixel(tile.startX+1, tile.startY+2, on);
	drawPixel(tile.startX+1, tile.startY+3, on);
	drawPixel(tile.startX+1, tile.startY+4, on);
	drawPixel(tile.startX+1, tile.startY+5, on);
	drawPixel(tile.startX+1, tile.startY+6, on);
	
	//top right
	drawPixel(tile.endX-1, tile.startY+1, on);
	drawPixel(tile.endX-2, tile.startY+1, on);
	drawPixel(tile.endX-3, tile.startY+1, on);
	drawPixel(tile.endX-4, tile.startY+1, on);
	drawPixel(tile.endX-5, tile.startY+1, on);
	drawPixel(tile.endX-6, tile.startY+1, on);
	
	drawPixel(tile.endX-1, tile.startY+1, on);
	drawPixel(tile.endX-1, tile.startY+2, on);
	drawPixel(tile.endX-1, tile.startY+3, on);
	drawPixel(tile.endX-1, tile.startY+4, on);
	drawPixel(tile.endX-1, tile.startY+5, on);
	drawPixel(tile.endX-1, tile.startY+6, on);
	
	//Bottom left
	drawPixel(tile.startX+1, tile.endY-1, on);
	drawPixel(tile.startX+2, tile.endY-1, on);
	drawPixel(tile.startX+3, tile.endY-1, on);
	drawPixel(tile.startX+4, tile.endY-1, on);
	drawPixel(tile.startX+5, tile.endY-1, on);
	drawPixel(tile.startX+6, tile.endY-1, on);
	
	drawPixel(tile.startX+1, tile.endY-1, on);
	drawPixel(tile.startX+1, tile.endY-2, on);
	drawPixel(tile.startX+1, tile.endY-3, on);
	drawPixel(tile.startX+1, tile.endY-4, on);
	drawPixel(tile.startX+1, tile.endY-5, on);
	drawPixel(tile.startX+1, tile.endY-6, on);
	
	//bottom right
	drawPixel(tile.endX-1, tile.endY-1, on);
	drawPixel(tile.endX-2, tile.endY-1, on);
	drawPixel(tile.endX-3, tile.endY-1, on);
	drawPixel(tile.endX-4, tile.endY-1, on);
	drawPixel(tile.endX-5, tile.endY-1, on);
	drawPixel(tile.endX-6, tile.endY-1, on);
	
	drawPixel(tile.endX-1, tile.endY-1, on);
	drawPixel(tile.endX-1, tile.endY-2, on);
	drawPixel(tile.endX-1, tile.endY-3, on);
	drawPixel(tile.endX-1, tile.endY-4, on);
	drawPixel(tile.endX-1, tile.endY-5, on);
	drawPixel(tile.endX-1, tile.endY-6, on);
}

void updateUI() {
	for (int i = 0; i < mainRenderer.tileCount; i++) {
		//For every tile, if it's currently rendering, draw the frame
		//If it is NOT rendering, clear any frame present
		if (mainRenderer.renderTiles[i].isRendering) {
			drawFrame(mainRenderer.renderTiles[i], true);
		} else {
			drawFrame(mainRenderer.renderTiles[i], false);
		}
	}
}

void drawWindow() {
	SDL_SetRenderDrawColor(mainDisplay.renderer, 0x0, 0x0, 0x0, 0x0);
	SDL_RenderClear(mainDisplay.renderer);
	//Check for CTRL-C
	if (signal(SIGINT, sigHandler) == SIG_ERR)
		fprintf(stderr, "Couldn't catch SIGINT\n");
	//Render frame
	updateUI();
	//Update image data
	SDL_UpdateTexture(mainDisplay.texture, NULL, mainRenderer.worldScene->camera->imgData, mainRenderer.worldScene->camera->width * 3);
	SDL_UpdateTexture(mainDisplay.overlayTexture, NULL, mainRenderer.uiBuffer, mainRenderer.worldScene->camera->width * 4);
	SDL_RenderCopy(mainDisplay.renderer, mainDisplay.texture, NULL, NULL);
	SDL_RenderCopy(mainDisplay.renderer, mainDisplay.overlayTexture, NULL, NULL);
	SDL_RenderPresent(mainDisplay.renderer);
}
