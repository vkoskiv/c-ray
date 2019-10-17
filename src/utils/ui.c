//
//  ui.c
//  C-ray
//
//  Created by Valtteri Koskivuori on 19/02/2017.
//  Copyright © 2015-2019 Valtteri Koskivuori. All rights reserved.
//

#include "../includes.h"
#include "ui.h"

#include "../renderer/renderer.h"
#include "../utils/logging.h"
#include "../datatypes/tile.h"
#include "../datatypes/texture.h"

//Signal handling
void (*signal(int signo, void (*func )(int)))(int);
typedef void sigfunc(int);
sigfunc *signal(int, sigfunc*);

void sigHandler(int sig) {
	if (sig == SIGINT) {
		logr(info, "Received ^C, aborting...\n");
		exit(1);
	}
}

void printDuration(float time) {
	printf("\n");
	logr(info, "Finished render in ");
	if (time <= 60) {
		printf("%.0f seconds.\n", time);
	} else if (time <= 3600) {
		printf("%.0f minute", time/60);
		if (time/60 > 1) printf("s. (%.0f seconds)\n", time); else printf(". (%.0f seconds)\n", time);
	} else {
		printf("%.0f hours (%.0f min).\n", (time/60)/60, time/60);
	}
}

#ifdef UI_ENABLED
int initSDL(struct display *d) {
	
	if (!d->enabled) {
		return 0;
	}
	
	//Initialize SDL
	if (SDL_Init(SDL_INIT_VIDEO) < 0) {
		logr(warning, "SDL couldn't initialize, error: \"%s\"\n", SDL_GetError());
		return -1;
	}
	//Init window
	SDL_WindowFlags flags = SDL_WINDOW_SHOWN | SDL_WINDOW_ALLOW_HIGHDPI;
	if (d->isFullScreen) flags |= SDL_WINDOW_FULLSCREEN_DESKTOP;
	if (d->isBorderless) flags |= SDL_WINDOW_BORDERLESS;
	flags |= SDL_WINDOW_RESIZABLE;
	
	d->window = SDL_CreateWindow("C-ray © VKoskiv 2015-2019",
								 SDL_WINDOWPOS_UNDEFINED,
								 SDL_WINDOWPOS_UNDEFINED,
								 d->width * d->windowScale,
								 d->height * d->windowScale,
								 flags);
	if (d->window == NULL) {
		logr(warning, "Window couldn't be created, error: \"%s\"\n", SDL_GetError());
		return -1;
	}
	//Init renderer
	d->renderer = SDL_CreateRenderer(d->window, -1, SDL_RENDERER_ACCELERATED);
	if (d->renderer == NULL) {
		logr(warning, "Renderer couldn't be created, error: \"%s\"\n", SDL_GetError());
		return -1;
	}
	
	SDL_RenderSetLogicalSize(d->renderer, d->width, d->height);
	//And set blend modes
	SDL_SetRenderDrawBlendMode(d->renderer, SDL_BLENDMODE_BLEND);
	
	SDL_RenderSetScale(d->renderer, d->windowScale, d->windowScale);
	//Init pixel texture
	d->texture = SDL_CreateTexture(d->renderer, SDL_PIXELFORMAT_RGB24, SDL_TEXTUREACCESS_STREAMING, d->width, d->height);
	if (d->texture == NULL) {
		logr(warning, "Texture couldn't be created, error: \"%s\"\n", SDL_GetError());
		return -1;
	}
	//Init overlay texture (for UI info)
	d->overlayTexture = SDL_CreateTexture(d->renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STREAMING, d->width, d->height);
	if (d->overlayTexture == NULL) {
		logr(warning, "Overlay texture couldn't be created, error: \"%s\"\n", SDL_GetError());
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
			if (event.key.keysym.sym == SDLK_s) {
				logr(info, "Aborting render, saving...\n");
				r->state.renderAborted = true;
			}
			if (event.key.keysym.sym == SDLK_x) {
				logr(info, "Aborting render without saving...\n");
				r->prefs.fileMode = saveModeNone;
				r->state.renderAborted = true;
			}
			if (event.key.keysym.sym == SDLK_p) {
				
				if (r->state.threadPaused[0]) {
					logr(info, "Resuming render.\n");
				} else {
					printf("\n");
					logr(info, "Pausing render.\n");
				}
				
				for (int i = 0; i < r->prefs.threadCount; i++) {
					if (r->state.threadPaused[i]) {
						r->state.threadPaused[i] = false;
					} else {
						r->state.threadPaused[i] = true;
					}
				}
			}
		}
	}
}

void drawPixel(struct renderer *r, int x, int y, bool on, struct color c) {
	if (y <= 0) y = 1;
	if (x <= 0) x = 1;
	if (x >= *r->state.image->width) x = *r->state.image->width - 1;
	if (y >= *r->state.image->height) y = *r->state.image->height - 1;
	
	if (on) {
		r->state.uiBuffer[(x + (*r->state.image->height - (y+1))
							   * *r->state.image->width) * 4 + 3] = (unsigned char)min(c.red*255.0, 255.0);
		r->state.uiBuffer[(x + (*r->state.image->height - (y+1))
							   * *r->state.image->width) * 4 + 2] = (unsigned char)min(c.green*255.0, 255.0);
		r->state.uiBuffer[(x + (*r->state.image->height - (y+1))
							   * *r->state.image->width) * 4 + 1] = (unsigned char)min(c.blue*255.0, 255.0);
		r->state.uiBuffer[(x + (*r->state.image->height - (y+1))
							   * *r->state.image->width) * 4 + 0] = (unsigned char)min(255.0, 255.0);
	} else {
		r->state.uiBuffer[(x + (*r->state.image->height - (y+1)) * *r->state.image->width) * 4 + 0] = (unsigned char)0;
		r->state.uiBuffer[(x + (*r->state.image->height - (y+1)) * *r->state.image->width) * 4 + 1] = (unsigned char)0;
		r->state.uiBuffer[(x + (*r->state.image->height - (y+1)) * *r->state.image->width) * 4 + 2] = (unsigned char)0;
		r->state.uiBuffer[(x + (*r->state.image->height - (y+1)) * *r->state.image->width) * 4 + 3] = (unsigned char)0;
	}
}

void clearProgBar(struct renderer *r, struct renderTile temp) {
	for (int i = 0; i < temp.width; i++) {
		drawPixel(r, temp.begin.x + i, (temp.begin.y + (temp.height/5)) - 1, false, progColor);
		drawPixel(r, temp.begin.x + i, (temp.begin.y + (temp.height/5)), false, progColor);
		drawPixel(r, temp.begin.x + i, (temp.begin.y + (temp.height/5)) + 1, false, progColor);
	}
}

/*
 So this is a bit of a kludge, we get the dynamically updated completedSamples
 info that renderThreads report back, and then associate that with the static
 renderTiles array data that is only updated once a tile is completed.
 I didn't want to put any mutex locks in the main render loop, so this gets
 around that.
 */
void drawProgressBars(struct renderer *r) {
	for (int t = 0; t < r->prefs.threadCount; t++) {
		if (r->state.renderThreadInfo[t].currentTileNum != -1) {
			struct renderTile temp = r->state.renderTiles[r->state.renderThreadInfo[t].currentTileNum];
			int completedSamples = r->state.renderThreadInfo[t].completedSamples;
			int totalSamples = r->prefs.sampleCount;
			
			float prc = ((float)completedSamples / (float)totalSamples);
			int pixels2draw = (int)((float)temp.width*(float)prc);
			
			//And then draw the bar
			for (int i = 0; i < pixels2draw; i++) {
				drawPixel(r, temp.begin.x + i, (temp.begin.y + (temp.height/5)) - 1, true, progColor);
				drawPixel(r, temp.begin.x + i, (temp.begin.y + (temp.height/5)), true, progColor);
				drawPixel(r, temp.begin.x + i, (temp.begin.y + (temp.height/5)) + 1, true, progColor);
			}
		}
	}
}

/**
 Draw highlight frame to show which tiles are rendering

 @param tile Given renderTile
 @param on Draw frame if true, erase if false
 */
void drawFrame(struct renderer *r, struct renderTile tile, bool on) {
	int length = 8;
	if (tile.width < 16) length = 4;
	for (int i = 1; i < length; i++) {
		//top left
		drawPixel(r, tile.begin.x+i, tile.begin.y+1, on, frameColor);
		drawPixel(r, tile.begin.x+1, tile.begin.y+i, on, frameColor);
		
		//top right
		drawPixel(r, tile.end.x-i, tile.begin.y+1, on, frameColor);
		drawPixel(r, tile.end.x-1, tile.begin.y+i, on, frameColor);
		
		//Bottom left
		drawPixel(r, tile.begin.x+i, tile.end.y-1, on, frameColor);
		drawPixel(r, tile.begin.x+1, tile.end.y-i, on, frameColor);
		
		//bottom right
		drawPixel(r, tile.end.x-i, tile.end.y-1, on, frameColor);
		drawPixel(r, tile.end.x-1, tile.end.y-i, on, frameColor);
	}
}

void updateFrames(struct renderer *r) {
	if (r->prefs.tileWidth < 8 || r->prefs.tileHeight < 8) return;
	for (int i = 0; i < r->state.tileCount; i++) {
		//For every tile, if it's currently rendering, draw the frame
		//If it is NOT rendering, clear any frame present
		drawFrame(r, r->state.renderTiles[i], r->state.renderTiles[i].isRendering);
		if (r->state.renderTiles[i].renderComplete) {
			clearProgBar(r, r->state.renderTiles[i]);
		}
	}
	drawProgressBars(r);
}

void drawWindow(struct renderer *r) {
	//Check for CTRL-C
	if (signal(SIGINT, sigHandler) == SIG_ERR)
		logr(warning, "Couldn't catch SIGINT\n");
	//Render frames
	updateFrames(r);
	//Update image data
	SDL_UpdateTexture(r->mainDisplay->texture, NULL, r->state.image->byte_data, *r->state.image->width * 3);
	SDL_UpdateTexture(r->mainDisplay->overlayTexture, NULL, r->state.uiBuffer, *r->state.image->width * 4);
	SDL_RenderCopy(r->mainDisplay->renderer, r->mainDisplay->texture, NULL, NULL);
	SDL_RenderCopy(r->mainDisplay->renderer, r->mainDisplay->overlayTexture, NULL, NULL);
	SDL_RenderPresent(r->mainDisplay->renderer);
}

void freeDisplay(struct display *disp) {
	if (disp->window) {
		SDL_DestroyWindow(disp->window);
	}
	if (disp->renderer) {
		SDL_DestroyRenderer(disp->renderer);
	}
	if (disp->texture) {
		SDL_DestroyTexture(disp->texture);
	}
	if (disp->overlayTexture) {
		SDL_DestroyTexture(disp->overlayTexture);
	}
}

#endif
