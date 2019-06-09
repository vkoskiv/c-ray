//
//  texture.c
//  C-ray
//
//  Created by Valtteri on 09/04/2019.
//  Copyright © 2015-2019 Valtteri Koskivuori. All rights reserved.
//

#include "texture.h"

#include "../includes.h"

//Note how imageData only stores 8-bit precision for each color channel.
//This is why we use the renderBuffer (blitDouble) for the running average as it just contains
//the full precision color values
void blit(struct texture *t, struct color *c, unsigned int x, unsigned int y) {
	t->data[(x + (*t->height - y)* *t->width)*3 + 0] =
	(unsigned char)min( max(c->red*255.0,0), 255.0);
	t->data[(x + (*t->height - y)* *t->width)*3 + 1] =
	(unsigned char)min( max(c->green*255.0,0), 255.0);
	t->data[(x + (*t->height - y)* *t->width)*3 + 2] =
	(unsigned char)min( max(c->blue*255.0,0), 255.0);
}

void blitDouble(double *buf, int width, int height, struct color *c, unsigned int x, unsigned int y) {
	buf[(x + (height - y)*width)*3 + 0] = c->red;
	buf[(x + (height - y)*width)*3 + 1] = c->green;
	buf[(x + (height - y)*width)*3 + 2] = c->blue;
}
