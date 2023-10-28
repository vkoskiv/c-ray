//
//  ui.h
//  C-ray
//
//  Created by Valtteri Koskivuori on 19/02/2017.
//  Copyright © 2017-2020 Valtteri Koskivuori. All rights reserved.
//

#pragma once

struct renderer;
struct texture;
struct sdl_preview;

#ifdef CRAY_SDL_ENABLED
	#include "SDL.h"
#endif

void try_init_win(struct sdl_preview *win, int width, int height);
void destroyDisplay(void);

void printDuration(uint64_t ms);
void getKeyboardInput(struct renderer *r);
void drawWindow(struct renderer *r, struct texture *t);
