//
//  display.c
//  C-Ray
//
//  Created by Valtteri Koskivuori on 31/12/2016.
//  Copyright © 2016 Valtteri Koskivuori. All rights reserved.
//

#include "display.h"

void destroyWindow(SDL_Window *window) {
	if (window != NULL) {
		SDL_DestroyWindow(window);
	}
}

void destroyRenderer(SDL_Renderer *renderer) {
	if (renderer != NULL) {
		SDL_DestroyRenderer(renderer);
	}
}

void destroyTexture(SDL_Texture *texture) {
	if (texture != NULL) {
		SDL_DestroyTexture(texture);
	}
}

/*void render(SDL_Renderer *renderer) {
	//Clear the window first
	SDL_SetRenderDrawColor(renderer, 0x0, 0x0, 0x0, 0x0);
	SDL_RenderClear(renderer);
	
	int width = 1280;
	int height = 720;
	//Render the current image data array
	char data[width*height*3];
	getFrameData(data);
}*/

void renderPixel(int x, int y, color color) {
	//Do stuff
}
