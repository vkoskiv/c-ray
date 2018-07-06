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
#include "tile.h"

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

int initSDL(struct display *d) {
	
	//Initialize SDL
	if (SDL_Init(SDL_INIT_VIDEO) < 0) {
		logr(warning, "SDL couldn't initialize, error %s\n", SDL_GetError());
		return -1;
	}
	//Init window
	SDL_WindowFlags flags = SDL_WINDOW_SHOWN | SDL_WINDOW_ALLOW_HIGHDPI;
	if (d->isFullScreen) flags |= SDL_WINDOW_FULLSCREEN_DESKTOP;
	if (d->isBorderless) flags |= SDL_WINDOW_BORDERLESS;
	
	d->window = SDL_CreateWindow("C-ray © VKoskiv 2015-2018",
								 SDL_WINDOWPOS_UNDEFINED,
								 SDL_WINDOWPOS_UNDEFINED,
								 d->width * d->windowScale,
								 d->height * d->windowScale,
								 flags);
	if (d->window == NULL) {
		logr(warning, "Window couldn't be created, error %s\n", SDL_GetError());
		return -1;
	}
	//Init renderer
	d->renderer = SDL_CreateRenderer(d->window, -1, SDL_RENDERER_ACCELERATED);
	if (d->renderer == NULL) {
		logr(warning, "Renderer couldn't be created, error %s\n", SDL_GetError());
		return -1;
	}
	//And set blend modes
	SDL_SetRenderDrawBlendMode(d->renderer, SDL_BLENDMODE_BLEND);
	
	SDL_RenderSetScale(d->renderer, d->windowScale, d->windowScale);
	//Init pixel texture
	d->texture = SDL_CreateTexture(d->renderer, SDL_PIXELFORMAT_RGB24, SDL_TEXTUREACCESS_STREAMING, d->width, d->height);
	if (d->texture == NULL) {
		logr(warning, "Texture couldn't be created, error %s\n", SDL_GetError());
		return -1;
	}
	//Init overlay texture (for UI info)
	d->overlayTexture = SDL_CreateTexture(d->renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STREAMING, d->width, d->height);
	if (d->overlayTexture == NULL) {
		logr(warning, "Overlay texture couldn't be created, error %s\n", SDL_GetError());
		return -1;
	}
	
	//And set blend modes for textures too
	SDL_SetTextureBlendMode(d->texture, SDL_BLENDMODE_BLEND);
	SDL_SetTextureBlendMode(d->overlayTexture, SDL_BLENDMODE_BLEND);
	
	//Set window icon
	//setWindowIcon(d->window);
	
	return 0;
}

void getKeyboardInput(struct renderer *r) {
	SDL_Event event;
	while (SDL_PollEvent(&event)) {
		if (event.type == SDL_KEYDOWN && event.key.repeat == 0) {
			//const Uint8 *keys = SDL_GetKeyboardState(NULL);
			if (event.key.keysym.sym == SDLK_s) {
				logr(info, "Aborting render, saving...\n");
				r->renderAborted = true;
			}
			if (event.key.keysym.sym == SDLK_x) {
				logr(info, "Aborting render without saving...\n");
				r->mode = saveModeNone;
				r->renderAborted = true;
			}
			if (event.key.keysym.sym == SDLK_p) {
				
				if (r->threadPaused[0]) {
					logr(info, "Resuming render.\n");
				} else {
					printf("\n");
					logr(info, "Pausing render.\n");
				}
				
				for (int i = 0; i < r->threadCount; i++) {
					if (r->threadPaused[i]) {
						r->threadPaused[i] = false;
					} else {
						r->threadPaused[i] = true;
					}
				}
			}
		}
	}
}

void drawPixel(struct renderer *r, int x, int y, bool on) {
	if (y <= 0) y = 1;
	if (x <= 0) x = 1;
	if (x >= r->image->size.width) x = r->image->size.width - 1;
	if (y >= r->image->size.height) y = r->image->size.height - 1;
	
	if (on) {
		r->uiBuffer[(x + (r->image->size.height - y)
							   * r->image->size.width) * 4 + 3] = (unsigned char)min(frameColor.red*255.0, 255.0);
		r->uiBuffer[(x + (r->image->size.height - y)
							   * r->image->size.width) * 4 + 2] = (unsigned char)min(frameColor.green*255.0, 255.0);
		r->uiBuffer[(x + (r->image->size.height - y)
							   * r->image->size.width) * 4 + 1] = (unsigned char)min(frameColor.blue*255.0, 255.0);
		r->uiBuffer[(x + (r->image->size.height - y)
							   * r->image->size.width) * 4 + 0] = (unsigned char)min(255.0, 255.0);
	} else {
		r->uiBuffer[(x + (r->image->size.height - y) * r->image->size.width) * 4 + 0] = (unsigned char)0;
		r->uiBuffer[(x + (r->image->size.height - y) * r->image->size.width) * 4 + 1] = (unsigned char)0;
		r->uiBuffer[(x + (r->image->size.height - y) * r->image->size.width) * 4 + 2] = (unsigned char)0;
		r->uiBuffer[(x + (r->image->size.height - y) * r->image->size.width) * 4 + 3] = (unsigned char)0;
	}
}


/**
 Draw highlight frame to show which tiles are rendering

 @param tile Given renderTile
 @param on Draw frame if true, erase if false
 */
void drawFrame(struct renderer *r, struct renderTile tile, bool on) {
	for (int i = 1; i < 7; i++) {
		//top left
		drawPixel(r, tile.begin.x+i, tile.begin.y+1, on);
		drawPixel(r, tile.begin.x+1, tile.begin.y+i, on);
		
		//top right
		drawPixel(r, tile.end.x-i, tile.begin.y+1, on);
		drawPixel(r, tile.end.x-1, tile.begin.y+i, on);
		
		//Bottom left
		drawPixel(r, tile.begin.x+i, tile.end.y-1, on);
		drawPixel(r, tile.begin.x+1, tile.end.y-i, on);
		
		//bottom right
		drawPixel(r, tile.end.x-i, tile.end.y-1, on);
		drawPixel(r, tile.end.x-1, tile.end.y-i, on);
	}
}

void updateFrames(struct renderer *r) {
	for (int i = 0; i < r->tileCount; i++) {
		//For every tile, if it's currently rendering, draw the frame
		//If it is NOT rendering, clear any frame present
		if (r->renderTiles[i].isRendering) {
			drawFrame(r, r->renderTiles[i], true);
		} else {
			drawFrame(r, r->renderTiles[i], false);
		}
	}
}

void drawWindow(struct renderer *r) {
	//Check for CTRL-C
	if (signal(SIGINT, sigHandler) == SIG_ERR)
		logr(warning, "Couldn't catch SIGINT\n");
	//Render frames
	updateFrames(r);
	//Update image data
	SDL_UpdateTexture(r->mainDisplay->texture, NULL, r->image->data, r->image->size.width * 3);
	SDL_UpdateTexture(r->mainDisplay->overlayTexture, NULL, r->uiBuffer, r->image->size.width * 4);
	SDL_RenderCopy(r->mainDisplay->renderer, r->mainDisplay->texture, NULL, NULL);
	SDL_RenderCopy(r->mainDisplay->renderer, r->mainDisplay->overlayTexture, NULL, NULL);
	SDL_RenderPresent(r->mainDisplay->renderer);
}

#endif
