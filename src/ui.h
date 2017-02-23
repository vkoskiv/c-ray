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

typedef struct {
	SDL_Window *window;
	SDL_Renderer *renderer;
	SDL_Texture *texture;
	SDL_Texture *overlayTexture;
	pthread_mutex_t uiMutex;
	pthread_attr_t uiThreadAttributes;
	threadInfo *uiThreadInfo;
}display;

int initSDL();
void printDuration(double time);
void getKeyboardInput();
void *drawThread(void *arg);

extern display mainDisplay;

#endif /* ui_h */
