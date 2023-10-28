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
struct sdl_prefs;

#ifdef CRAY_SDL_ENABLED
	#include "SDL.h"
#endif

struct window *win_try_init(struct sdl_prefs *prefs, int width, int height);
void win_destroy(struct window *);

void printDuration(uint64_t ms);
void getKeyboardInput(struct renderer *r);
void win_update(struct window *w, struct renderer *r, struct texture *t);
