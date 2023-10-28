//
//  ui.c
//  C-ray
//
//  Created by Valtteri Koskivuori on 19/02/2017.
//  Copyright © 2017-2021 Valtteri Koskivuori. All rights reserved.
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

struct window {
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

// FIXME: Move the sig handling elsewhere, no reason for it to be tied to this SDL stuff
static bool g_aborted = false;

//FIXME: This won't work on linux, it'll just abort the execution.
//Take a look at the docs for sigaction() and implement that.
void sigHandler(int sig) {
	if (sig == 2) { //SIGINT
		logr(plain, "\n");
		logr(info, "Received ^C, aborting render without saving\n");
		g_aborted = true;
	}
}

#ifdef CRAY_SDL_ENABLED

static void setWindowIcon(SDL_Window *window) {
#ifndef NO_LOGO
	struct texture *icon = load_texture_from_buffer(logo_png_data, logo_png_data_len, NULL);
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

struct window *win_try_init(struct sdl_prefs *prefs, int width, int height) {
#ifdef CRAY_SDL_ENABLED
	if (!prefs->enabled) return NULL;
	struct window *display = calloc(1, sizeof(*display));

	display->isFullScreen = prefs->fullscreen;
	display->isBorderless = prefs->borderless;
	display->windowScale = prefs->scale;
	display->width = width;
	display->height = height;
	
	//Initialize SDL
	if (SDL_Init(SDL_INIT_VIDEO) < 0) {
		logr(warning, "SDL couldn't initialize, error: \"%s\"\n", SDL_GetError());
		win_destroy(display);
		return NULL;
	}
	//Init window
	SDL_WindowFlags flags = SDL_WINDOW_SHOWN | SDL_WINDOW_ALLOW_HIGHDPI;
	if (prefs->fullscreen) flags |= SDL_WINDOW_FULLSCREEN_DESKTOP;
	if (prefs->borderless) flags |= SDL_WINDOW_BORDERLESS;
	flags |= SDL_WINDOW_RESIZABLE;

	display->window = SDL_CreateWindow("c-ray © vkoskiv 2015-2023",
										 SDL_WINDOWPOS_UNDEFINED,
										 SDL_WINDOWPOS_UNDEFINED,
								 width * prefs->scale,
								 height * prefs->scale,
								 flags);
	if (display->window == NULL) {
		logr(warning, "Window couldn't be created, error: \"%s\"\n", SDL_GetError());
		win_destroy(display);
		return NULL;
	}
	//Init renderer
	display->renderer = SDL_CreateRenderer(display->window, -1, SDL_RENDERER_ACCELERATED);
	if (display->renderer == NULL) {
		logr(warning, "Renderer couldn't be created, error: \"%s\"\n", SDL_GetError());
		win_destroy(display);
		return NULL;
	}
	
	SDL_RenderSetLogicalSize(display->renderer, display->width, display->height);
	//And set blend modes
	SDL_SetRenderDrawBlendMode(display->renderer, SDL_BLENDMODE_BLEND);
	
	SDL_RenderSetScale(display->renderer, display->windowScale, display->windowScale);
	//Init pixel texture
	display->texture = SDL_CreateTexture(display->renderer, SDL_PIXELFORMAT_RGB24, SDL_TEXTUREACCESS_STREAMING, display->width, display->height);
	if (display->texture == NULL) {
		logr(warning, "Texture couldn't be created, error: \"%s\"\n", SDL_GetError());
		win_destroy(display);
		return NULL;
	}
	//Init overlay texture (for UI info)

	uint32_t format = SDL_PIXELFORMAT_ABGR8888;
#if SDL_BYTEORDER == BIG_ENDIAN
	format = SDL_PIXELFORMAT_RGBA8888;
#endif
	display->overlayTexture = SDL_CreateTexture(display->renderer, format, SDL_TEXTUREACCESS_STREAMING, display->width, display->height);
	if (display->overlayTexture == NULL) {
		logr(warning, "Overlay texture couldn't be created, error: \"%s\"\n", SDL_GetError());
		win_destroy(display);
		return NULL;
	}
	
	//And set blend modes for textures too
	SDL_SetTextureBlendMode(display->texture, SDL_BLENDMODE_BLEND);
	SDL_SetTextureBlendMode(display->overlayTexture, SDL_BLENDMODE_BLEND);
	
	setWindowIcon(display->window);
	
	return display;
#else
	(void)prefs; (void)width; (void)height;
	logr(warning, "Render preview is disabled. (No SDL2)\n");
	return NULL;
#endif
}

void win_destroy(struct window *window) {
#ifdef CRAY_SDL_ENABLED
	if (!window) return;
	SDL_Quit();
	if (window->texture) {
		SDL_DestroyTexture(window->texture);
		window->texture = NULL;
	}
	if (window->overlayTexture) {
		SDL_DestroyTexture(window->overlayTexture);
		window->texture = NULL;
	}
	if (window->renderer) {
		SDL_DestroyRenderer(window->renderer);
		window->renderer = NULL;
	}
	if (window->window) {
		SDL_DestroyWindow(window->window);
		window->window = NULL;
	}
	free(window);
	window = NULL;
#endif
}

void printDuration(uint64_t ms) {
	logr(info, "Finished render in ");
	printSmartTime(ms);
	logr(plain, "                     \n");
}

void getKeyboardInput(struct renderer *r) {
	if (g_aborted) {
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
		if (event.type == SDL_QUIT) {
			printf("\n");
			logr(info, "Aborting render without saving\n");
			r->state.renderAborted = true;
			r->state.saveImage = false;
		}
	}
#endif
}

#ifdef CRAY_SDL_ENABLED
static void clearProgBar(struct renderer *r, struct renderTile temp) {
	for (unsigned i = 0; i < temp.width; ++i) {
		setPixel(r->state.uiBuffer, g_clear_color, temp.begin.x + i, (temp.begin.y + (temp.height / 5)) - 1);
		setPixel(r->state.uiBuffer, g_clear_color, temp.begin.x + i, (temp.begin.y + (temp.height / 5))    );
		setPixel(r->state.uiBuffer, g_clear_color, temp.begin.x + i, (temp.begin.y + (temp.height / 5)) + 1);
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
		if (r->state.threadStates[t].currentTile) {
			struct renderTile *temp = r->state.threadStates[t].currentTile;
			int completedSamples = r->state.threadStates[t].completedSamples;
			int totalSamples = r->prefs.sampleCount;
			
			float prc = ((float)completedSamples / (float)totalSamples);
			int pixels2draw = (int)((float)temp->width * prc);

			struct color c = temp->state == rendering ? g_prog_color: g_clear_color;
			
			//And then draw the bar
			for (int i = 0; i < pixels2draw; ++i) {
				setPixel(r->state.uiBuffer, c, temp->begin.x + i, (temp->begin.y + (temp->height / 5)) - 1);
				setPixel(r->state.uiBuffer, c, temp->begin.x + i, (temp->begin.y + (temp->height / 5))    );
				setPixel(r->state.uiBuffer, c, temp->begin.x + i, (temp->begin.y + (temp->height / 5)) + 1);
			}
		}
	}
	for (int i = 0; i < r->state.tileCount; ++i) {
		if (r->state.renderTiles[i].state == finished) {
			clearProgBar(r, r->state.renderTiles[i]);
		}

	}
}

/**
 Draw highlight frame to show which tiles are rendering

 @param r Renderer
 @param tile Given renderTile
 */
static void drawFrame(struct texture *buf, struct renderTile tile, struct color c) {
	unsigned length = tile.width  <= 16 ? 4 : 8;
			 length = tile.height <= 16 ? 4 : 8;
	length = length > tile.width ? tile.width : length;
	length = length > tile.height ? tile.height : length;

	for (unsigned i = 1; i < length; ++i) {
		//top left
		setPixel(buf, c, tile.begin.x + i, tile.begin.y + 1);
		setPixel(buf, c, tile.begin.x + 1, tile.begin.y + i);
		
		//top right
		setPixel(buf, c, tile.end.x - i, tile.begin.y + 1);
		setPixel(buf, c, tile.end.x - 1, tile.begin.y + i);
		
		//Bottom left
		setPixel(buf, c, tile.begin.x + i, tile.end.y - 1);
		setPixel(buf, c, tile.begin.x + 1, tile.end.y - i);
		
		//bottom right
		setPixel(buf, c, tile.end.x - i, tile.end.y - 1);
		setPixel(buf, c, tile.end.x - 1, tile.end.y - i);
	}
}

static void updateFrames(struct renderer *r) {
	if (r->prefs.tileWidth < 8 || r->prefs.tileHeight < 8) return;
	for (int i = 0; i < r->state.tileCount; ++i) {
		struct renderTile tile = r->state.renderTiles[i];
		struct color c = tile.state == rendering ? g_frame_color : g_clear_color;
		drawFrame(r->state.uiBuffer, tile, c);
	}
}
#endif

void win_update(struct window *w, struct renderer *r, struct texture *t) {
	if (g_aborted) r->state.renderAborted = true;
#ifdef CRAY_SDL_ENABLED
	if (!w) return;
	//Render frames
	if (!isSet("interactive") || r->state.clients) {
		updateFrames(r);
		drawProgressBars(r);
	}
	//Update image data
	SDL_UpdateTexture(w->texture, NULL, t->data.byte_p, (int)t->width * 3);
	SDL_UpdateTexture(w->overlayTexture, NULL, r->state.uiBuffer->data.byte_p, (int)t->width * 4);
	SDL_RenderCopy(w->renderer, w->texture, NULL, NULL);
	SDL_RenderCopy(w->renderer, w->overlayTexture, NULL, NULL);
	SDL_RenderPresent(w->renderer);
#else
	(void)t;
#endif
}
