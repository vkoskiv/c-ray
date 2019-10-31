//
//  texture.c
//  C-ray
//
//  Created by Valtteri on 09/04/2019.
//  Copyright © 2015-2019 Valtteri Koskivuori. All rights reserved.
//

#include "../includes.h"

#include "texture.h"

vec3 textureGetPixel(struct texture *t, int x, int y);

<<<<<<< HEAD
//Note how imageData only stores 8-bit precision for each color channel.
//This is why we use the renderBuffer (blitfloat) for the running average as it just contains
//the full precision color values
void blit(struct texture *t, vec3 c, unsigned int x, unsigned int y) {
	if ((x > *t->width-1) || y < 0) return;
	if ((y > *t->height-1) || y < 0) return;
	t->byte_data[(x + (*t->height - (y+1))* *t->width)*3 + 0] = (unsigned char)min( max(c.r*255.0,0), 255.0);
	t->byte_data[(x + (*t->height - (y+1))* *t->width)*3 + 1] = (unsigned char)min( max(c.g*255.0,0), 255.0);
	t->byte_data[(x + (*t->height - (y+1))* *t->width)*3 + 2] = (unsigned char)min( max(c.b*255.0,0), 255.0);
}

void blitfloat(float *buf, int width, int height, vec3 *c, unsigned int x, unsigned int y) {
	buf[(x + (height - y)*width)*3 + 0] = c->r;
	buf[(x + (height - y)*width)*3 + 1] = c->g;
	buf[(x + (height - y)*width)*3 + 2] = c->b;
}

vec3 bilinearInterpolate(vec3 topleft, vec3 topright, vec3 botleft, vec3 botright, float tx, float ty) {
	return vec3_mix(vec3_mix(topleft, topright, tx), vec3_mix(botleft, botright, tx), ty);
=======
//General-purpose blit function
void blit(struct texture *t, struct color c, unsigned int x, unsigned int y) {
	if ((x > *t->width-1) || y < 0) return;
	if ((y > *t->height-1) || y < 0) return;
	if (t->precision == char_p) {
		t->byte_data[(x + (*t->height - (y+1))* *t->width)* *t->channels + 0] = (unsigned char)min(c.red*255.0, 255.0);
		t->byte_data[(x + (*t->height - (y+1))* *t->width)* *t->channels + 1] = (unsigned char)min(c.green*255.0,255.0);
		t->byte_data[(x + (*t->height - (y+1))* *t->width)* *t->channels + 2] = (unsigned char)min(c.blue*255.0,255.0);
		if (t->hasAlpha) t->byte_data[(x + (*t->height - (y+1))* *t->width)* *t->channels + 3] = (unsigned char)min(c.alpha*255.0, 255.0);
	} else if (t->precision == float_p) {
		t->float_data[(x + (*t->height - (y+1))* *t->width)* *t->channels + 0] = c.red;
		t->float_data[(x + (*t->height - (y+1))* *t->width)* *t->channels + 1] = c.green;
		t->float_data[(x + (*t->height - (y+1))* *t->width)* *t->channels + 2] = c.blue;
		if (t->hasAlpha) t->float_data[(x + (*t->height - (y+1))* *t->width)* *t->channels + 3] = c.alpha;
	}
>>>>>>> 1d60640fe22419135cd05015879227d4992e474f
}

//Bilinearly interpolated (smoothed) output. Requires float precision, i.e. 0.0->width-1.0
vec3 textureGetPixelFiltered(struct texture *t, float x, float y) {
	float xcopy = x - 0.5;
	float ycopy = y - 0.5;
	int xint = (int)xcopy;
	int yint = (int)ycopy;
<<<<<<< HEAD
	vec3 topleft = textureGetPixel(t, xint, yint);
	vec3 topright = textureGetPixel(t, xint + 1, yint);
	vec3 botleft = textureGetPixel(t, xint, yint + 1);
	vec3 botright = textureGetPixel(t, xint + 1, yint + 1);
	return bilinearInterpolate(topleft, topright, botleft, botright, xcopy-xint, ycopy-yint);
}

//FIXME: Use this everywhere, in renderer too where there is now a duplicate getPixel()
vec3 textureGetPixel(struct texture *t, int x, int y) {
	vec3 output = {0.0, 0.0, 0.0};
=======
	struct color topleft = textureGetPixel(t, xint, yint);
	struct color topright = textureGetPixel(t, xint + 1, yint);
	struct color botleft = textureGetPixel(t, xint, yint + 1);
	struct color botright = textureGetPixel(t, xint + 1, yint + 1);
	return lerp(lerp(topleft, topright, xcopy-xint), lerp(botleft, botright, xcopy-xint), ycopy-yint);
}

struct color textureGetPixel(struct texture *t, int x, int y) {
	struct color output = {0.0, 0.0, 0.0, 0.0};
>>>>>>> 1d60640fe22419135cd05015879227d4992e474f
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
	
<<<<<<< HEAD
	if (t->fileType == hdr) {
		output.r = t->float_data[(x + ((*t->height-1) - y) * *t->width)*pitch + 0];
		output.g = t->float_data[(x + ((*t->height-1) - y) * *t->width)*pitch + 1];
		output.b = t->float_data[(x + ((*t->height-1) - y) * *t->width)*pitch + 2];
		//output.a = t->hasAlpha ? t->float_data[(x + ((*t->height-1) - y) * *t->width)*pitch + 3] : 1.0;
=======
	if (t->precision == float_p) {
		output.red = t->float_data[(x + ((*t->height-1) - y) * *t->width)*pitch + 0];
		output.green = t->float_data[(x + ((*t->height-1) - y) * *t->width)*pitch + 1];
		output.blue = t->float_data[(x + ((*t->height-1) - y) * *t->width)*pitch + 2];
		output.alpha = t->hasAlpha ? t->float_data[(x + ((*t->height-1) - y) * *t->width)*pitch + 3] : 1.0;
>>>>>>> 1d60640fe22419135cd05015879227d4992e474f
	} else {
		output.r = t->byte_data[(x + ((*t->height-1) - y) * *t->width)*pitch + 0]/255.0;
		output.g = t->byte_data[(x + ((*t->height-1) - y) * *t->width)*pitch + 1]/255.0;
		output.b = t->byte_data[(x + ((*t->height-1) - y) * *t->width)*pitch + 2]/255.0;
		//output.alpha = t->hasAlpha ? t->byte_data[(x + (*t->height - y) * *t->width)*pitch + 3]/255.0 : 1.0;
	}
	
	return output;
}

struct texture *newTexture() {
	struct texture *t = calloc(1, sizeof(struct texture));
	t->byte_data = NULL;
	t->float_data = NULL;
	t->width = calloc(1, sizeof(unsigned int));
	t->height = calloc(1, sizeof(unsigned int));
	t->channels = calloc(1, sizeof(int));
	t->colorspace = linear;
	t->count = 0;
	t->hasAlpha = false;
	t->fileType = buffer;
	t->offset = 0.0f;
	return t;
}

void allocTextureBuffer(struct texture *t, enum precision p, int width, int height, int channels) {
	*t->width = width;
	*t->height = height;
	t->precision = p;
	*t->channels = channels;
	if (channels > 3) t->hasAlpha = true;
	
	switch (t->precision) {
		case char_p:
			t->byte_data = calloc(channels *width * height, sizeof(unsigned char));
			break;
		case float_p:
			t->float_data = calloc(channels * width * height, sizeof(float));
			break;
	}
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

void freeTexture(struct texture *t) {
	if (t->fileName) {
		free(t->fileName);
	}
	if (t->filePath) {
		free(t->filePath);
	}
	if (t->byte_data) {
		free(t->byte_data);
	}
	if (t->float_data) {
		free(t->float_data);
	}
	if (t->channels) {
		free(t->channels);
	}
	if (t->width) {
		free(t->width);
	}
	if (t->height) {
		free(t->height);
	}
}
