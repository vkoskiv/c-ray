//
//  ui.c
//  C-ray
//
//  Created by Valtteri Koskivuori on 19/02/2017.
//  Copyright © 2015-2020 Valtteri Koskivuori. All rights reserved.
//

#include "../includes.h"
#include "ui.h"

#include "../datatypes/image/imagefile.h"
#include "../renderer/renderer.h"
#include "logging.h"
#include "../datatypes/tile.h"
#include "../datatypes/image/texture.h"
#include "../datatypes/color.h"
#include "../utils/platform/thread.h"
#include "../utils/platform/signal.h"
#include "../utils/assert.h"

struct display {
#ifdef CRAY_SDL_ENABLED
	SDL_Window *window;
	SDL_Renderer *renderer;
	SDL_Texture *texture;
	SDL_Texture *overlayTexture;
#endif
	bool isBorderless;
	bool isFullScreen;
	float windowScale;
	
	unsigned width;
	unsigned height;
};

static bool aborted = false;

static struct display *gdisplay = NULL;

//FIXME: This won't work on linux, it'll just abort the execution.
//Take a look at the docs for sigaction() and implement that.
void sigHandler(int sig) {
	if (sig == 2) { //SIGINT
		printf("\n");
		logr(info, "Received ^C, aborting render without saving\n");
		aborted = true;
	}
}

void initDisplay(bool fullscreen, bool borderless, int width, int height, float scale) {
#ifdef CRAY_SDL_ENABLED
	ASSERT(!gdisplay);
	gdisplay = calloc(1, sizeof(struct display));
	
	gdisplay->isFullScreen = fullscreen;
	gdisplay->isBorderless = borderless;
	gdisplay->width = width;
	gdisplay->height = height;
	gdisplay->windowScale = scale;
	
	//Initialize SDL
	if (SDL_Init(SDL_INIT_VIDEO) < 0) {
		logr(warning, "SDL couldn't initialize, error: \"%s\"\n", SDL_GetError());
	}
	//Init window
	SDL_WindowFlags flags = SDL_WINDOW_SHOWN | SDL_WINDOW_ALLOW_HIGHDPI;
	if (fullscreen) flags |= SDL_WINDOW_FULLSCREEN_DESKTOP;
	if (borderless) flags |= SDL_WINDOW_BORDERLESS;
	flags |= SDL_WINDOW_RESIZABLE;
	
	gdisplay->window = SDL_CreateWindow("C-ray © VKoskiv 2015-2020",
								 SDL_WINDOWPOS_UNDEFINED,
								 SDL_WINDOWPOS_UNDEFINED,
								 width * scale,
								 height * scale,
								 flags);
	if (gdisplay->window == NULL) {
		logr(warning, "Window couldn't be created, error: \"%s\"\n", SDL_GetError());
	}
	//Init renderer
	gdisplay->renderer = SDL_CreateRenderer(gdisplay->window, -1, SDL_RENDERER_ACCELERATED);
	if (gdisplay->renderer == NULL) {
		logr(warning, "Renderer couldn't be created, error: \"%s\"\n", SDL_GetError());
	}
	
	SDL_RenderSetLogicalSize(gdisplay->renderer, gdisplay->width, gdisplay->height);
	//And set blend modes
	SDL_SetRenderDrawBlendMode(gdisplay->renderer, SDL_BLENDMODE_BLEND);
	
	SDL_RenderSetScale(gdisplay->renderer, gdisplay->windowScale, gdisplay->windowScale);
	//Init pixel texture
	gdisplay->texture = SDL_CreateTexture(gdisplay->renderer, SDL_PIXELFORMAT_RGB24, SDL_TEXTUREACCESS_STREAMING, gdisplay->width, gdisplay->height);
	if (gdisplay->texture == NULL) {
		logr(warning, "Texture couldn't be created, error: \"%s\"\n", SDL_GetError());
	}
	//Init overlay texture (for UI info)
	gdisplay->overlayTexture = SDL_CreateTexture(gdisplay->renderer, SDL_PIXELFORMAT_ABGR8888, SDL_TEXTUREACCESS_STREAMING, gdisplay->width, gdisplay->height);
	if (gdisplay->overlayTexture == NULL) {
		logr(warning, "Overlay texture couldn't be created, error: \"%s\"\n", SDL_GetError());
	}
	
	//And set blend modes for textures too
	SDL_SetTextureBlendMode(gdisplay->texture, SDL_BLENDMODE_BLEND);
	SDL_SetTextureBlendMode(gdisplay->overlayTexture, SDL_BLENDMODE_BLEND);
#else
	(void)fullscreen; (void)borderless; (void)width; (void)height; (void)scale;
	logr(warning, "Render preview is disabled. (No SDL2)\n");
#endif
}

void destroyDisplay() {
#ifdef CRAY_SDL_ENABLED
	ASSERT(gdisplay);
	if (gdisplay) {
		if (gdisplay->window) {
			SDL_DestroyWindow(gdisplay->window);
		}
		if (gdisplay->renderer) {
			SDL_DestroyRenderer(gdisplay->renderer);
		}
		if (gdisplay->texture) {
			SDL_DestroyTexture(gdisplay->texture);
		}
		if (gdisplay->overlayTexture) {
			SDL_DestroyTexture(gdisplay->overlayTexture);
		}
		free(gdisplay);
	}
#endif
}

void printDuration(uint64_t ms) {
	logr(info, "Finished render in ");
	printSmartTime(ms);
	printf("                     \n");
}

void getKeyboardInput(struct renderer *r) {
	if (aborted) {
		r->state.renderAborted = true;
	}
	static bool sigRegistered = false;
	//Check for CTRL-C
	if (!sigRegistered) {
		if (registerHandler(sigint, sigHandler)) {
			logr(warning, "Unable to catch SIGINT\n");
		}
		sigRegistered = true;
	}
#ifdef CRAY_SDL_ENABLED
	SDL_Event event;
	while (SDL_PollEvent(&event)) {
		if (event.type == SDL_KEYDOWN && event.key.repeat == 0) {
			if (event.key.keysym.sym == SDLK_s) {
				printf("\n");
				logr(info, "Aborting render, saving\n");
				r->state.renderAborted = true;
				r->state.saveImage = true;
			}
			if (event.key.keysym.sym == SDLK_x) {
				printf("\n");
				logr(info, "Aborting render without saving\n");
				r->state.renderAborted = true;
				r->state.saveImage = false;
			}
			if (event.key.keysym.sym == SDLK_p) {
				for (int i = 0; i < r->prefs.threadCount; ++i) {
					if (r->state.threads[i].paused) {
						r->state.threads[i].paused = false;
					} else {
						r->state.threads[i].paused = true;
					}
				}
			}
		}
	}
#endif
}

void clearProgBar(struct renderer *r, struct renderTile temp) {
	for (unsigned i = 0; i < temp.width; ++i) {
		setPixel(r->state.uiBuffer, clearColor, temp.begin.x + i, (temp.begin.y + (temp.height/5)) - 1);
		setPixel(r->state.uiBuffer, clearColor, temp.begin.x + i, (temp.begin.y + (temp.height/5))    );
		setPixel(r->state.uiBuffer, clearColor, temp.begin.x + i, (temp.begin.y + (temp.height/5)) + 1);
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
	for (int t = 0; t < r->prefs.threadCount; ++t) {
		if (r->state.threads[t].currentTileNum != -1) {
			struct renderTile temp = r->state.renderTiles[r->state.threads[t].currentTileNum];
			int completedSamples = r->state.threads[t].completedSamples;
			int totalSamples = r->prefs.sampleCount;
			
			float prc = ((float)completedSamples / (float)totalSamples);
			int pixels2draw = (int)((float)temp.width*(float)prc);
			
			//And then draw the bar
			for (int i = 0; i < pixels2draw; ++i) {
				setPixel(r->state.uiBuffer, progColor, temp.begin.x + i, (temp.begin.y + (temp.height/5)) - 1);
				setPixel(r->state.uiBuffer, progColor, temp.begin.x + i, (temp.begin.y + (temp.height/5))    );
				setPixel(r->state.uiBuffer, progColor, temp.begin.x + i, (temp.begin.y + (temp.height/5)) + 1);
			}
		}
	}
}

/**
 Draw highlight frame to show which tiles are rendering

 @param r Renderer
 @param tile Given renderTile
 */
void drawFrame(struct renderer *r, struct renderTile tile) {
	unsigned length = tile.width  <= 16 ? 4 : 8;
			 length = tile.height <= 16 ? 4 : 8;
	length = length > tile.width ? tile.width : length;
	length = length > tile.height ? tile.height : length;
	struct color c = clearColor;
	if (tile.isRendering) {
		c = frameColor;
	} else if (tile.renderComplete) {
		c = clearColor;
	} else {
		return;
	}
	for (unsigned i = 1; i < length; ++i) {
		//top left
		setPixel(r->state.uiBuffer, c, tile.begin.x+i, tile.begin.y+1);
		setPixel(r->state.uiBuffer, c, tile.begin.x+1, tile.begin.y+i);
		
		//top right
		setPixel(r->state.uiBuffer, c, tile.end.x-i, tile.begin.y+1);
		setPixel(r->state.uiBuffer, c, tile.end.x-1, tile.begin.y+i);
		
		//Bottom left
		setPixel(r->state.uiBuffer, c, tile.begin.x+i, tile.end.y-1);
		setPixel(r->state.uiBuffer, c, tile.begin.x+1, tile.end.y-i);
		
		//bottom right
		setPixel(r->state.uiBuffer, c, tile.end.x-i, tile.end.y-1);
		setPixel(r->state.uiBuffer, c, tile.end.x-1, tile.end.y-i);
	}
}

void updateFrames(struct renderer *r) {
	if (r->prefs.tileWidth < 8 || r->prefs.tileHeight < 8) return;
	for (int i = 0; i < r->state.tileCount; ++i) {
		//For every tile, if it's currently rendering, draw the frame
		//If it is NOT rendering, clear any frame present
		drawFrame(r, r->state.renderTiles[i]);
		if (r->state.renderTiles[i].renderComplete) {
			clearProgBar(r, r->state.renderTiles[i]);
		}
	}
	drawProgressBars(r);
}

void drawWindow(struct renderer *r, struct texture *t) {
	if (aborted) {
		r->state.renderAborted = true;
	}
#ifdef CRAY_SDL_ENABLED
	//Render frames
	updateFrames(r);
	//Update image data
	SDL_UpdateTexture(gdisplay->texture, NULL, t->data.byte_p, t->width * 3);
	SDL_UpdateTexture(gdisplay->overlayTexture, NULL, r->state.uiBuffer->data.byte_p, t->width * 4);
	SDL_RenderCopy(gdisplay->renderer, gdisplay->texture, NULL, NULL);
	SDL_RenderCopy(gdisplay->renderer, gdisplay->overlayTexture, NULL, NULL);
	SDL_RenderPresent(gdisplay->renderer);
#else
	(void)t;
#endif
}
