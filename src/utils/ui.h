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

void initDisplay(bool fullscreen, bool borderless, int width, int height, float scale);
void destroyDisplay(void);

void printDuration(uint64_t ms);
void getKeyboardInput(struct renderer *r);
void drawWindow(struct renderer *r, struct texture *t);
