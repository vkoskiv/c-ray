//
//  ui.h
//  C-ray
//
//  Created by Valtteri Koskivuori on 19/02/2017.
//  Copyright © 2015-2020 Valtteri Koskivuori. All rights reserved.
//

#pragma once

struct renderer;
struct texture;

#ifdef CRAY_SDL_ENABLED
	#include "SDL.h"
#endif

//FIXME: This should be in datatypes
struct display {
#ifdef CRAY_SDL_ENABLED
	SDL_Window *window;
	SDL_Renderer *renderer;
	SDL_Texture *texture;
	SDL_Texture *overlayTexture;
#endif
	bool enabled;
	bool isBorderless;
	bool isFullScreen;
	float windowScale;
	
	unsigned width;
	unsigned height;
};

int initSDL(struct display *d);
void destroyDisplay(struct display *d);

void printDuration(uint64_t ms);
void getKeyboardInput(struct renderer *r);
void drawWindow(struct renderer *r, struct texture *t);
