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
		printf("\n");
		logr(info, "Received ^C, aborting\n");
		exit(1);
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
	d->overlayTexture = SDL_CreateTexture(d->renderer, SDL_PIXELFORMAT_ABGR8888, SDL_TEXTUREACCESS_STREAMING, d->width, d->height);
	if (d->overlayTexture == NULL) {
		logr(warning, "Overlay texture couldn't be created, error: \"%s\"\n", SDL_GetError());
		return -1;
	}
	
	//And set blend modes for textures too
	SDL_SetTextureBlendMode(d->texture, SDL_BLENDMODE_BLEND);
	SDL_SetTextureBlendMode(d->overlayTexture, SDL_BLENDMODE_BLEND);
	
	return 0;
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

void printDuration(float time) {
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

void getKeyboardInput(struct renderer *r) {
#ifdef UI_ENABLED
	SDL_Event event;
	while (SDL_PollEvent(&event)) {
		if (event.type == SDL_KEYDOWN && event.key.repeat == 0) {
			if (event.key.keysym.sym == SDLK_s) {
				printf("\n");
				logr(info, "Aborting render, saving\n");
				r->state.renderAborted = true;
			}
			if (event.key.keysym.sym == SDLK_x) {
				printf("\n");
				logr(info, "Aborting render without saving\n");
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
#endif
}

void clearProgBar(struct renderer *r, struct renderTile temp) {
	for (int i = 0; i < temp.width; i++) {
		blit(r->state.uiBuffer, clearColor, temp.begin.x + i, (temp.begin.y + (temp.height/5)) - 1);
		blit(r->state.uiBuffer, clearColor, temp.begin.x + i, (temp.begin.y + (temp.height/5))    );
		blit(r->state.uiBuffer, clearColor, temp.begin.x + i, (temp.begin.y + (temp.height/5)) + 1);
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
		if (r->state.threadStates[t].currentTileNum != -1) {
			struct renderTile temp = r->state.renderTiles[r->state.threadStates[t].currentTileNum];
			int completedSamples = r->state.threadStates[t].completedSamples;
			int totalSamples = r->prefs.sampleCount;
			
			float prc = ((float)completedSamples / (float)totalSamples);
			int pixels2draw = (int)((float)temp.width*(float)prc);
			
			//And then draw the bar
			for (int i = 0; i < pixels2draw; i++) {
				blit(r->state.uiBuffer, progColor, temp.begin.x + i, (temp.begin.y + (temp.height/5)) - 1);
				blit(r->state.uiBuffer, progColor, temp.begin.x + i, (temp.begin.y + (temp.height/5))    );
				blit(r->state.uiBuffer, progColor, temp.begin.x + i, (temp.begin.y + (temp.height/5)) + 1);
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
	struct color c = clearColor;
	if (tile.isRendering) {
		c = frameColor;
	}
	if (tile.width < 16) length = 4;
	for (int i = 1; i < length; i++) {
		//top left
		blit(r->state.uiBuffer, c, tile.begin.x+i, tile.begin.y+1);
		blit(r->state.uiBuffer, c, tile.begin.x+1, tile.begin.y+i);
		
		//top right
		blit(r->state.uiBuffer, c, tile.end.x-i, tile.begin.y+1);
		blit(r->state.uiBuffer, c, tile.end.x-1, tile.begin.y+i);
		
		//Bottom left
		blit(r->state.uiBuffer, c, tile.begin.x+i, tile.end.y-1);
		blit(r->state.uiBuffer, c, tile.begin.x+1, tile.end.y-i);
		
		//bottom right
		blit(r->state.uiBuffer, c, tile.end.x-i, tile.end.y-1);
		blit(r->state.uiBuffer, c, tile.end.x-1, tile.end.y-i);
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
#ifdef UI_ENABLED
	//Check for CTRL-C
	if (signal(SIGINT, sigHandler) == SIG_ERR)
		logr(warning, "Couldn't catch SIGINT\n");
	//Render frames
	updateFrames(r);
	//Update image data
	SDL_UpdateTexture(r->mainDisplay->texture, NULL, r->state.image->byte_data, r->state.image->width * 3);
	SDL_UpdateTexture(r->mainDisplay->overlayTexture, NULL, r->state.uiBuffer->byte_data, r->state.image->width * 4);
	SDL_RenderCopy(r->mainDisplay->renderer, r->mainDisplay->texture, NULL, NULL);
	SDL_RenderCopy(r->mainDisplay->renderer, r->mainDisplay->overlayTexture, NULL, NULL);
	SDL_RenderPresent(r->mainDisplay->renderer);
#endif
}
