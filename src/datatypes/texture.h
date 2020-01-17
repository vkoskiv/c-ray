//
//  texture.h
//  C-ray
//
//  Created by Valtteri on 09/04/2019.
//  Copyright © 2015-2020 Valtteri Koskivuori. All rights reserved.
//

struct dimensions {
	int height;
	int width;
};

enum colorspace {
	linear,
	sRGB
};

enum precision {
	char_p,
	float_p,
	none
};

struct renderInfo {
	int samples;
	int bounces;
	char *renderTime;
	int threadCount;
	char *arch;
	char *crayVersion;
	char *gitHash;
};

struct texture {
	bool hasAlpha;
	enum fileType fileType;
	enum colorspace colorspace;
	enum precision precision;
	char *filePath;
	char *fileName;
	int count;
	unsigned char *byte_data; //For 24/32bit
	float *float_data; //For hdr
	int channels; //For hdr
	float offset; //radians, for hdr
	unsigned width;
	unsigned height;
};

struct color;

struct texture *newTexture(enum precision p, int width, int height, int channels);

void blit(struct texture *t, struct color c, unsigned int x, unsigned int y);
struct color textureGetPixel(struct texture *t, unsigned x, unsigned y);
struct color textureGetPixelFiltered(struct texture *t, float x, float y);
void textureFromSRGB(struct texture *t);
void textureToSRGB(struct texture *t);

void freeTexture(struct texture *tex);
