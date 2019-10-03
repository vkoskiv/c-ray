//
//  texture.c
//  C-ray
//
//  Created by Valtteri on 09/04/2019.
//  Copyright © 2015-2019 Valtteri Koskivuori. All rights reserved.
//

#include "../includes.h"

#include "texture.h"

struct color textureGetPixel(struct texture *t, int x, int y);

//Note how imageData only stores 8-bit precision for each color channel.
//This is why we use the renderBuffer (blitDouble) for the running average as it just contains
//the full precision color values
void blit(struct texture *t, struct color c, unsigned int x, unsigned int y) {
	if ((x > *t->width-1) || y < 0) return;
	if ((y > *t->height-1) || y < 0) return;
	t->byte_data[(x + (*t->height - (y+1))* *t->width)*3 + 0] = (unsigned char)min( max(c.red*255.0,0), 255.0);
	t->byte_data[(x + (*t->height - (y+1))* *t->width)*3 + 1] = (unsigned char)min( max(c.green*255.0,0), 255.0);
	t->byte_data[(x + (*t->height - (y+1))* *t->width)*3 + 2] = (unsigned char)min( max(c.blue*255.0,0), 255.0);
}

void blitDouble(double *buf, int width, int height, struct color *c, unsigned int x, unsigned int y) {
	buf[(x + (height - y)*width)*3 + 0] = c->red;
	buf[(x + (height - y)*width)*3 + 1] = c->green;
	buf[(x + (height - y)*width)*3 + 2] = c->blue;
}

struct color lerp(struct color start, struct color end, float t) {
	struct color ret = {0};
	ret.red = start.red + (end.red - start.red) * t;
	ret.green = start.green + (end.green - start.green) * t;
	ret.blue = start.blue + (end.blue - start.blue) * t;
	ret.alpha = start.alpha + (end.alpha - start.alpha) * t;
	return ret;
}

struct color bilinearInterpolate(struct color topleft, struct color topright, struct color botleft, struct color botright, float tx, float ty) {
	return lerp(lerp(topleft, topright, tx), lerp(botleft, botright, tx), ty);
}

//Bilinearly interpolated (smoothed) output. Requires float precision, i.e. 0.0->width-1.0
struct color textureGetPixelFiltered(struct texture *t, float x, float y) {
	float xcopy = x - 0.5;
	float ycopy = y - 0.5;
	int xint = (int)xcopy;
	int yint = (int)ycopy;
	struct color topleft = textureGetPixel(t, xint, yint);
	struct color topright = textureGetPixel(t, xint + 1, yint);
	struct color botleft = textureGetPixel(t, xint, yint + 1);
	struct color botright = textureGetPixel(t, xint + 1, yint + 1);
	return bilinearInterpolate(topleft, topright, botleft, botright, xcopy-xint, ycopy-yint);
}

//FIXME: Use this everywhere, in renderer too where there is now a duplicate getPixel()
struct color textureGetPixel(struct texture *t, int x, int y) {
	struct color output = {0.0, 0.0, 0.0, 0.0};
	int pitch = 0;
	if (t->hasAlpha) {
		pitch = 4;
	} else {
		pitch = 3;
	}
	
	//bilinear lerp might tweak the values, so just clamp here to be safe.
	x = x > *t->width-1 ? *t->width-1 : x;
	y = y > *t->height-1 ? *t->height-1 : y;
	x = x < 0 ? 0 : x;
	y = y < 0 ? 0 : y;
	
	if (t->fileType == hdr) {
		output.red = t->float_data[(x + ((*t->height-1) - y) * *t->width)*pitch + 0];
		output.green = t->float_data[(x + ((*t->height-1) - y) * *t->width)*pitch + 1];
		output.blue = t->float_data[(x + ((*t->height-1) - y) * *t->width)*pitch + 2];
		output.alpha = t->hasAlpha ? t->float_data[(x + ((*t->height-1) - y) * *t->width)*pitch + 3] : 1.0;
	} else {
		output.red   = t->byte_data[(x + ((*t->height-1) - y) * *t->width)*pitch + 0]/255.0;
		output.green = t->byte_data[(x + ((*t->height-1) - y) * *t->width)*pitch + 1]/255.0;
		output.blue  = t->byte_data[(x + ((*t->height-1) - y) * *t->width)*pitch + 2]/255.0;
		output.alpha = t->hasAlpha ? t->byte_data[(x + (*t->height - y) * *t->width)*pitch + 3]/255.0 : 1.0;
	}
	
	return output;
}

void textureFromSRGB(struct texture *t) {
	if (t->colorspace == sRGB) return;
	for (int x = 0; x < *t->width; x++) {
		for (int y = 0; y < *t->height; y++) {
			blit(t, fromSRGB(textureGetPixel(t, x, y)), x, y);
		}
	}
	t->colorspace = linear;
}

void textureToSRGB(struct texture *t) {
	if (t->colorspace == linear) return;
	for (int x = 0; x < *t->width; x++) {
		for (int y = 0; y < *t->height; y++) {
			blit(t, toSRGB(textureGetPixel(t, x, y)), x, y);
		}
	}
	t->colorspace = sRGB;
}

void freeTexture(struct texture *tex) {
	if (tex->fileName) {
		free(tex->fileName);
	}
	if (tex->filePath) {
		free(tex->filePath);
	}
	if (tex->byte_data) {
		free(tex->byte_data);
	}
	if (tex->float_data) {
		free(tex->float_data);
	}
	if (tex->channels) {
		free(tex->channels);
	}
	if (tex->width) {
		free(tex->width);
	}
	if (tex->height) {
		free(tex->height);
	}
}
