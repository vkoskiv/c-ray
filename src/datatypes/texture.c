//
//  texture.c
//  C-ray
//
//  Created by Valtteri on 09/04/2019.
//  Copyright © 2015-2019 Valtteri Koskivuori. All rights reserved.
//

#include "../includes.h"

#include "texture.h"

color textureGetPixel(struct texture *t, int x, int y);

//General-purpose blit function
void blit(struct texture *t, color c, unsigned int x, unsigned int y) {
	if ((x > *t->width-1) || y < 0) return;
	if ((y > *t->height-1) || y < 0) return;

	if (t->precision == char_p) {
		t->byte_data[(x + (*t->height - (y + 1)) * *t->width) * *t->channels + 0] = (unsigned char)min(c.red * 255.0, 255.0);
		t->byte_data[(x + (*t->height - (y + 1)) * *t->width) * *t->channels + 1] = (unsigned char)min(c.green * 255.0, 255.0);
		t->byte_data[(x + (*t->height - (y + 1)) * *t->width) * *t->channels + 2] = (unsigned char)min(c.blue * 255.0, 255.0);
		if (t->hasAlpha) t->byte_data[(x + (*t->height - (y + 1)) * *t->width) * *t->channels + 3] = (unsigned char)min(c.alpha * 255.0, 255.0);
	}
	else if (t->precision == float_p) {
		t->float_data[(x + (*t->height - (y + 1)) * *t->width) * *t->channels + 0] = c.red;
		t->float_data[(x + (*t->height - (y + 1)) * *t->width) * *t->channels + 1] = c.green;
		t->float_data[(x + (*t->height - (y + 1)) * *t->width) * *t->channels + 2] = c.blue;
		if (t->hasAlpha) t->float_data[(x + (*t->height - (y + 1)) * *t->width) * *t->channels + 3] = c.alpha;
	}
}

color bilinearInterpolate(color topleft, color topright, color botleft, color botright, float tx, float ty) {
	return mixColors(mixColors(topleft, topright, tx), mixColors(botleft, botright, tx), ty);
}

//Bilinearly interpolated (smoothed) output. Requires float precision, i.e. 0.0->width-1.0
color textureGetPixelFiltered(struct texture *t, float x, float y) {
	float xcopy = x - 0.5;
	float ycopy = y - 0.5;
	int xint = (int)xcopy;
	int yint = (int)ycopy;
	color topleft = textureGetPixel(t, xint, yint);
	color topright = textureGetPixel(t, xint + 1, yint);
	color botleft = textureGetPixel(t, xint, yint + 1);
	color botright = textureGetPixel(t, xint + 1, yint + 1);
	return bilinearInterpolate(topleft, topright, botleft, botright, xcopy-xint, ycopy-yint);
}

//FIXME: Use this everywhere, in renderer too where there is now a duplicate getPixel()
color textureGetPixel(struct texture *t, int x, int y) {
	color output = {0.0f, 0.0f, 0.0f, 0.0f};

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
	
	if (t->fileType == hdr || t->precision == float_p) {
		output.red = t->float_data[(x + ((*t->height-1) - y) * *t->width)*pitch + 0];
		output.green = t->float_data[(x + ((*t->height-1) - y) * *t->width)*pitch + 1];
		output.blue = t->float_data[(x + ((*t->height-1) - y) * *t->width)*pitch + 2];
		output.alpha = t->hasAlpha ? t->float_data[(x + ((*t->height-1) - y) * *t->width)*pitch + 3] : 1.0;
	} else {
		output.red = t->byte_data[(x + ((*t->height-1) - y) * *t->width)*pitch + 0]/255.0;
		output.green = t->byte_data[(x + ((*t->height-1) - y) * *t->width)*pitch + 1]/255.0;
		output.blue = t->byte_data[(x + ((*t->height-1) - y) * *t->width)*pitch + 2]/255.0;
		output.alpha = t->hasAlpha ? t->byte_data[(x + (*t->height - y) * *t->width)*pitch + 3]/255.0 : 1.0;
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
			t->byte_data = calloc(channels * width * height, sizeof(unsigned char));
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

float gaussian(float x, float y, float rho)
{
	float rho2 = rho * rho;
	return 1.0f / ((M_PI + M_PI) * rho2) * exp( -(x*x + y*y)/(rho2 + rho2) );
}

void blurTextureGaussian(struct texture* dst, struct texture* src, unsigned int kernelSize, float stdDev, bool doWrap) {
	// Gaussian blur over whole image

	int w = *dst->width, h = *dst->height;

	for (int y = 0; y < h; ++y)
	{
		for (int x = 0; x < w; ++x)
		{
			// Assume we're working with float data
			color newColor = blackColor;

			for (int gy = 0; gy < kernelSize; ++gy)
			{
				for (int gx = 0; gx < kernelSize; ++gx)
				{
					// sx, sy : real sampling location
					float sx = (float)x + (gx - (float)(kernelSize)*0.5f), sy = (float)y + (gy - (float)(kernelSize)*0.5f);

					if (doWrap)
					{
						sx = fmodf(sx, (float)w);
						sy = fmodf(sy, (float)h);
					}

					// u, v : [0;1]
					float u = (float)gx / (float)(kernelSize);
					float v = (float)gy / (float)(kernelSize);

					// px, py [-1;+1]
					float px = v * 2.0f - 1.0f;
					float py = v * 2.0f - 1.0f;

					float g = gaussian(px, py, stdDev);
					color colorSample = textureGetPixelFiltered(src, sx, sy);

					newColor = addColors(newColor, colorCoef(colorSample, g));
				}
			}

			blit(dst, newColor, x, y);
		}
	}
}