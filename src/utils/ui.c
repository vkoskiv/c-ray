//
//  ui.c
//  C-ray
//
//  Created by Valtteri Koskivuori on 19/02/2017.
//  Copyright © 2017-2020 Valtteri Koskivuori. All rights reserved.
//

#include "../includes.h"
#include "ui.h"

#include "../datatypes/image/imagefile.h"
#include "../renderer/renderer.h"
#include "logging.h"
#include "../datatypes/tile.h"
#include "../datatypes/image/texture.h"
#include "../datatypes/color.h"
#include "platform/thread.h"
#include "platform/signal.h"
#include "assert.h"
#include "logo.h"
#include "loaders/textureloader.h"
#include "args.h"

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

//FIXME: This won't work on linux, it'll just abort the execution.
//Take a look at the docs for sigaction() and implement that.
void sigHandler(int sig) {
	if (sig == 2) { //SIGINT
		logr(plain, "\n");
		logr(info, "Received ^C, aborting render without saving\n");
		aborted = true;
	}
}

#ifdef CRAY_SDL_ENABLED

static struct display *gdisplay = NULL;

static void setWindowIcon(SDL_Window *window) {
#ifndef NO_LOGO
	struct texture *icon = loadTextureFromBuffer(logo_png_data, logo_png_data_len, NULL);
	Uint32 rmask;
	Uint32 gmask;
	Uint32 bmask;
	Uint32 amask;
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
	int shift = (icon->channels == 3) ? 8 : 0;
	rmask = 0xff000000 >> shift;
	gmask = 0x00ff0000 >> shift;
	bmask = 0x0000ff00 >> shift;
	amask = 0x000000ff >> shift;
#else
	rmask = 0x000000ff;
	gmask = 0x0000ff00;
	bmask = 0x00ff0000;
	amask = (icon->channels == 3) ? 0 : 0xff000000;
#endif
	SDL_Surface *iconSurface = SDL_CreateRGBSurfaceFrom(icon->data.byte_p,
														(int)icon->width,
														(int)icon->height,
														(int)icon->channels * 8,
														(int)(icon->channels * icon->width),
														rmask, gmask, bmask, amask);
	SDL_SetWindowIcon(window, iconSurface);
	SDL_FreeSurface(iconSurface);
	destroyTexture(icon);
#endif
}
#endif

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
		destroyDisplay();
		return;
	}
	//Init window
	SDL_WindowFlags flags = SDL_WINDOW_SHOWN | SDL_WINDOW_ALLOW_HIGHDPI;
	if (fullscreen) flags |= SDL_WINDOW_FULLSCREEN_DESKTOP;
	if (borderless) flags |= SDL_WINDOW_BORDERLESS;
	flags |= SDL_WINDOW_RESIZABLE;
	
	gdisplay->window = SDL_CreateWindow("C-ray © VKoskiv 2015-2021",
								 SDL_WINDOWPOS_UNDEFINED,
								 SDL_WINDOWPOS_UNDEFINED,
								 width * scale,
								 height * scale,
								 flags);
	if (gdisplay->window == NULL) {
		logr(warning, "Window couldn't be created, error: \"%s\"\n", SDL_GetError());
		destroyDisplay();
		return;
	}
	//Init renderer
	gdisplay->renderer = SDL_CreateRenderer(gdisplay->window, -1, SDL_RENDERER_ACCELERATED);
	if (gdisplay->renderer == NULL) {
		logr(warning, "Renderer couldn't be created, error: \"%s\"\n", SDL_GetError());
		destroyDisplay();
		return;
	}
	
	SDL_RenderSetLogicalSize(gdisplay->renderer, gdisplay->width, gdisplay->height);
	//And set blend modes
	SDL_SetRenderDrawBlendMode(gdisplay->renderer, SDL_BLENDMODE_BLEND);
	
	SDL_RenderSetScale(gdisplay->renderer, gdisplay->windowScale, gdisplay->windowScale);
	//Init pixel texture
	gdisplay->texture = SDL_CreateTexture(gdisplay->renderer, SDL_PIXELFORMAT_RGB24, SDL_TEXTUREACCESS_STREAMING, gdisplay->width, gdisplay->height);
	if (gdisplay->texture == NULL) {
		logr(warning, "Texture couldn't be created, error: \"%s\"\n", SDL_GetError());
		destroyDisplay();
		return;
	}
	//Init overlay texture (for UI info)
	gdisplay->overlayTexture = SDL_CreateTexture(gdisplay->renderer, SDL_PIXELFORMAT_ABGR8888, SDL_TEXTUREACCESS_STREAMING, gdisplay->width, gdisplay->height);
	if (gdisplay->overlayTexture == NULL) {
		logr(warning, "Overlay texture couldn't be created, error: \"%s\"\n", SDL_GetError());
		destroyDisplay();
		return;
	}
	
	//And set blend modes for textures too
	SDL_SetTextureBlendMode(gdisplay->texture, SDL_BLENDMODE_BLEND);
	SDL_SetTextureBlendMode(gdisplay->overlayTexture, SDL_BLENDMODE_BLEND);
	
	setWindowIcon(gdisplay->window);
	
#else
	(void)fullscreen; (void)borderless; (void)width; (void)height; (void)scale;
	logr(warning, "Render preview is disabled. (No SDL2)\n");
#endif
}

void destroyDisplay() {
#ifdef CRAY_SDL_ENABLED
	if (gdisplay) {
		SDL_Quit();
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
		gdisplay = NULL;
	}
#endif
}

void printDuration(uint64_t ms) {
	logr(info, "Finished render in ");
	printSmartTime(ms);
	logr(plain, "                     \n");
}

void getKeyboardInput(struct renderer *r) {
	if (aborted) {
		r->state.saveImage = false;
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
					r->state.threadStates[i].paused = !r->state.threadStates[i].paused;
				}
			}
		}
	}
#endif
}

#ifdef CRAY_SDL_ENABLED
static void clearProgBar(struct renderer *r, struct renderTile temp) {
	for (unsigned i = 0; i < temp.width; ++i) {
		setPixel(r->state.uiBuffer, clearColor, temp.begin.x + i, (temp.begin.y + (temp.height / 5)) - 1);
		setPixel(r->state.uiBuffer, clearColor, temp.begin.x + i, (temp.begin.y + (temp.height / 5))    );
		setPixel(r->state.uiBuffer, clearColor, temp.begin.x + i, (temp.begin.y + (temp.height / 5)) + 1);
	}
}

/*
 So this is a bit of a kludge, we get the dynamically updated completedSamples
 info that renderThreads report back, and then associate that with the static
 renderTiles array data that is only updated once a tile is completed.
 I didn't want to put any mutex locks in the main render loop, so this gets
 around that.
 */
static void drawProgressBars(struct renderer *r) {
	for (int t = 0; t < r->prefs.threadCount; ++t) {
		if (r->state.threadStates[t].currentTileNum != -1) {
			struct renderTile temp = r->state.renderTiles[r->state.threadStates[t].currentTileNum];
			int completedSamples = r->state.threadStates[t].completedSamples;
			int totalSamples = r->prefs.sampleCount;
			
			float prc = ((float)completedSamples / (float)totalSamples);
			int pixels2draw = (int)((float)temp.width * (float)prc);
			
			//And then draw the bar
			for (int i = 0; i < pixels2draw; ++i) {
				setPixel(r->state.uiBuffer, progColor, temp.begin.x + i, (temp.begin.y + (temp.height / 5)) - 1);
				setPixel(r->state.uiBuffer, progColor, temp.begin.x + i, (temp.begin.y + (temp.height / 5))    );
				setPixel(r->state.uiBuffer, progColor, temp.begin.x + i, (temp.begin.y + (temp.height / 5)) + 1);
			}
		}
	}
}

/**
 Draw highlight frame to show which tiles are rendering

 @param r Renderer
 @param tile Given renderTile
 */
static void drawFrame(struct renderer *r, struct renderTile tile) {
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
		setPixel(r->state.uiBuffer, c, tile.begin.x + i, tile.begin.y + 1);
		setPixel(r->state.uiBuffer, c, tile.begin.x + 1, tile.begin.y + i);
		
		//top right
		setPixel(r->state.uiBuffer, c, tile.end.x - i, tile.begin.y + 1);
		setPixel(r->state.uiBuffer, c, tile.end.x - 1, tile.begin.y + i);
		
		//Bottom left
		setPixel(r->state.uiBuffer, c, tile.begin.x + i, tile.end.y - 1);
		setPixel(r->state.uiBuffer, c, tile.begin.x + 1, tile.end.y - i);
		
		//bottom right
		setPixel(r->state.uiBuffer, c, tile.end.x - i, tile.end.y - 1);
		setPixel(r->state.uiBuffer, c, tile.end.x - 1, tile.end.y - i);
	}
}

static void updateFrames(struct renderer *r) {
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
#endif

void drawWindow(struct renderer *r, struct texture *t) {
	if (aborted) {
		r->state.renderAborted = true;
	}
#ifdef CRAY_SDL_ENABLED
	if (!gdisplay) return;
	//Render frames
	if (!isSet("interactive")) updateFrames(r);
	//Update image data
	SDL_UpdateTexture(gdisplay->texture, NULL, t->data.byte_p, (int)t->width * 3);
	SDL_UpdateTexture(gdisplay->overlayTexture, NULL, r->state.uiBuffer->data.byte_p, (int)t->width * 4);
	SDL_RenderCopy(gdisplay->renderer, gdisplay->texture, NULL, NULL);
	SDL_RenderCopy(gdisplay->renderer, gdisplay->overlayTexture, NULL, NULL);
	SDL_RenderPresent(gdisplay->renderer);
#else
	(void)t;
#endif
}
