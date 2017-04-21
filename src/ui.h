//
//  ui.h
//  C-Ray
//
//  Created by Valtteri Koskivuori on 19/02/2017.
//  Copyright © 2017 Valtteri Koskivuori. All rights reserved.
//

#ifndef ui_h
#define ui_h

#include "includes.h"
#include "renderer.h"

#ifdef UI_ENABLED
typedef struct {
	SDL_Window *window;
	SDL_Renderer *renderer;
	SDL_Texture *texture;
	SDL_Texture *overlayTexture;
	bool isBorderless;
	bool isFullScreen;
}display;

int initSDL();
void drawWindow();
extern display mainDisplay;
#endif

void printDuration(double time);
void getKeyboardInput();

#endif /* ui_h */
