//
//  ui.c
//  C-Ray
//
//  Created by Valtteri Koskivuori on 19/02/2017.
//  Copyright © 2017 Valtteri Koskivuori. All rights reserved.
//

#include "includes.h"
#include "ui.h"

#include "camera.h"
#include "renderer.h"
#include "scene.h"
#include "filehandler.h"
#include "logging.h"

extern struct renderer mainRenderer;

//Signal handling
void (*signal(int signo, void (*func )(int)))(int);
typedef void sigfunc(int);
sigfunc *signal(int, sigfunc*);

void sigHandler(int sig) {
	if (sig == SIGINT) {
		logr(info, "Received CTRL-C, aborting...\n");
		exit(1);
	}
}

void printDuration(double time) {
	if (time <= 60) {
		logr(info, "Finished render in %.0f seconds.\n", time);
	} else if (time <= 3600) {
		logr(info, "Finished render in %.0f minute", time/60);
		if (time/60 > 1) printf("s. (%.0f seconds)\n", time); else printf(". (%.0f seconds)\n", time);
	} else {
		logr(info, "Finished render in %.0f hours (%.0f min).\n", (time/60)/60, time/60);
	}
}

#ifdef UI_ENABLED
void fillTexture(SDL_Renderer *renderer, SDL_Texture *texture, int r, int g, int b, int a) {
	SDL_SetRenderTarget(renderer, texture);
	SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
	SDL_SetRenderDrawColor(renderer, r, g, b, a);
	SDL_RenderFillRect(renderer, NULL);
}

/*static void setWindowIcon(SDL_Window *window) {
	//For logo bitmap
	#include "logo.c"
	
	Uint32 rmask;
	Uint32 gmask;
	Uint32 bmask;
	Uint32 amask;
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
	int shift = (crayIcon.bytes_per_pixel == 3) ? 8 : 0;
	rmask = 0xff000000 >> shift;
	gmask = 0x00ff0000 >> shift;
	bmask = 0x0000ff00 >> shift;
	amask = 0x000000ff >> shift;
#else
	rmask = 0x000000ff;
	gmask = 0x0000ff00;
	bmask = 0x00ff0000;
	amask = (crayIcon.bytes_per_pixel == 3) ? 0 : 0xff000000;
#endif
	
	SDL_Surface *icon = SDL_CreateRGBSurfaceFrom((void*)crayIcon.pixel_data, crayIcon.width, crayIcon.height, crayIcon.bytes_per_pixel*8, crayIcon.bytes_per_pixel*crayIcon.width, rmask, gmask, bmask, amask);
	SDL_SetWindowIcon(window, icon);
	SDL_FreeSurface(icon);
}*/

int initSDL() {
	
	//Initialize SDL
	if (SDL_Init(SDL_INIT_VIDEO) < 0) {
		logr(warning, "SDL couldn't initialize, error %s\n", SDL_GetError());
		return -1;
	}
	//Init window
	SDL_WindowFlags flags = SDL_WINDOW_SHOWN | SDL_WINDOW_ALLOW_HIGHDPI;
	if (mainRenderer.mainDisplay->isFullScreen) flags |= SDL_WINDOW_FULLSCREEN_DESKTOP;
	if (mainRenderer.mainDisplay->isBorderless) flags |= SDL_WINDOW_BORDERLESS;
	
	mainRenderer.mainDisplay->window = SDL_CreateWindow("C-ray © VKoskiv 2015-2018", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, mainRenderer.image->size.width * mainRenderer.mainDisplay->windowScale, mainRenderer.image->size.height * mainRenderer.mainDisplay->windowScale, flags);
	if (mainRenderer.mainDisplay->window == NULL) {
		logr(warning, "Window couldn't be created, error %s\n", SDL_GetError());
		return -1;
	}
	//Init renderer
	mainRenderer.mainDisplay->renderer = SDL_CreateRenderer(mainRenderer.mainDisplay->window, -1, SDL_RENDERER_ACCELERATED);
	if (mainRenderer.mainDisplay->renderer == NULL) {
		logr(warning, "Renderer couldn't be created, error %s\n", SDL_GetError());
		return -1;
	}
	//And set blend modes
	SDL_SetRenderDrawBlendMode(mainRenderer.mainDisplay->renderer, SDL_BLENDMODE_BLEND);
	
	SDL_RenderSetScale(mainRenderer.mainDisplay->renderer, mainRenderer.mainDisplay->windowScale, mainRenderer.mainDisplay->windowScale);
	//Init pixel texture
	mainRenderer.mainDisplay->texture = SDL_CreateTexture(mainRenderer.mainDisplay->renderer, SDL_PIXELFORMAT_RGB24, SDL_TEXTUREACCESS_STREAMING, mainRenderer.image->size.width, mainRenderer.image->size.height);
	if (mainRenderer.mainDisplay->texture == NULL) {
		logr(warning, "Texture couldn't be created, error %s\n", SDL_GetError());
		return -1;
	}
	//Init overlay texture (for UI info)
	mainRenderer.mainDisplay->overlayTexture = SDL_CreateTexture(mainRenderer.mainDisplay->renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STREAMING, mainRenderer.image->size.width, mainRenderer.image->size.height);
	if (mainRenderer.mainDisplay->overlayTexture == NULL) {
		logr(warning, "Overlay texture couldn't be created, error %s\n", SDL_GetError());
		return -1;
	}
	
	//And set blend modes for textures too
	SDL_SetTextureBlendMode(mainRenderer.mainDisplay->texture, SDL_BLENDMODE_BLEND);
	SDL_SetTextureBlendMode(mainRenderer.mainDisplay->overlayTexture, SDL_BLENDMODE_BLEND);
	
	//Set window icon
	//setWindowIcon(mainRenderer.mainDisplay->window);
	
	return 0;
}

void getKeyboardInput() {
	SDL_PumpEvents();
	const Uint8 *keys = SDL_GetKeyboardState(NULL);
	if (keys[SDL_SCANCODE_S]) {
		logr(info, "Aborting render, saving...\n");
		mainRenderer.renderAborted = true;
	}
	if (keys[SDL_SCANCODE_X]) {
		logr(info, "Aborting render without saving...\n");
		mainRenderer.mode = saveModeNone;
		mainRenderer.renderAborted = true;
	}
	if (keys[SDL_SCANCODE_P]) {
		if (mainRenderer.renderPaused) {
			logr(info, "Resuming render...\n");
			mainRenderer.renderPaused = false;
		} else {
			logr(info, "Pausing render...\n");
			mainRenderer.renderPaused = true;
		}
	}
}

void drawPixel(int x, int y, bool on) {
	if (y <= 0) y = 1;
	if (x <= 0) x = 1;
	if (x >= mainRenderer.image->size.width) x = mainRenderer.image->size.width - 1;
	if (y >= mainRenderer.image->size.height) y = mainRenderer.image->size.height - 1;
	
	if (on) {
		mainRenderer.uiBuffer[(x + (mainRenderer.image->size.height - y)
							   * mainRenderer.image->size.width) * 4 + 3] = (unsigned char)min(frameColor.red*255.0, 255.0);
		mainRenderer.uiBuffer[(x + (mainRenderer.image->size.height - y)
							   * mainRenderer.image->size.width) * 4 + 2] = (unsigned char)min(frameColor.green*255.0, 255.0);
		mainRenderer.uiBuffer[(x + (mainRenderer.image->size.height - y)
							   * mainRenderer.image->size.width) * 4 + 1] = (unsigned char)min(frameColor.blue*255.0, 255.0);
		mainRenderer.uiBuffer[(x + (mainRenderer.image->size.height - y)
							   * mainRenderer.image->size.width) * 4 + 0] = (unsigned char)min(255.0, 255.0);
	} else {
		mainRenderer.uiBuffer[(x + (mainRenderer.image->size.height - y) * mainRenderer.image->size.width) * 4 + 0] = (unsigned char)0;
		mainRenderer.uiBuffer[(x + (mainRenderer.image->size.height - y) * mainRenderer.image->size.width) * 4 + 1] = (unsigned char)0;
		mainRenderer.uiBuffer[(x + (mainRenderer.image->size.height - y) * mainRenderer.image->size.width) * 4 + 2] = (unsigned char)0;
		mainRenderer.uiBuffer[(x + (mainRenderer.image->size.height - y) * mainRenderer.image->size.width) * 4 + 3] = (unsigned char)0;
	}
}


/**
 Draw highlight frame to show which tiles are rendering

 @param tile Given renderTile
 @param on Draw frame if true, erase if false
 */
void drawFrame(struct renderTile tile, bool on) {
	for (int i = 1; i < 7; i++) {
		//top left
		drawPixel(tile.begin.x+i, tile.begin.y+1, on);
		drawPixel(tile.begin.x+1, tile.begin.y+i, on);
		
		//top right
		drawPixel(tile.end.x-i, tile.begin.y+1, on);
		drawPixel(tile.end.x-1, tile.begin.y+i, on);
		
		//Bottom left
		drawPixel(tile.begin.x+i, tile.end.y-1, on);
		drawPixel(tile.begin.x+1, tile.end.y-i, on);
		
		//bottom right
		drawPixel(tile.end.x-i, tile.end.y-1, on);
		drawPixel(tile.end.x-1, tile.end.y-i, on);
	}
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
	SDL_SetRenderDrawColor(mainRenderer.mainDisplay->renderer, 0x0, 0x0, 0x0, 0x0);
	SDL_RenderClear(mainRenderer.mainDisplay->renderer);
	//Check for CTRL-C
	if (signal(SIGINT, sigHandler) == SIG_ERR)
		logr(warning, "Couldn't catch SIGINT\n");
	//Render frame
	updateUI();
	//Update image data
	SDL_UpdateTexture(mainRenderer.mainDisplay->texture, NULL, mainRenderer.image->data, mainRenderer.image->size.width * 3);
	SDL_UpdateTexture(mainRenderer.mainDisplay->overlayTexture, NULL, mainRenderer.uiBuffer, mainRenderer.image->size.width * 4);
	SDL_RenderCopy(mainRenderer.mainDisplay->renderer, mainRenderer.mainDisplay->texture, NULL, NULL);
	SDL_RenderCopy(mainRenderer.mainDisplay->renderer, mainRenderer.mainDisplay->overlayTexture, NULL, NULL);
	SDL_RenderPresent(mainRenderer.mainDisplay->renderer);
}

#endif
