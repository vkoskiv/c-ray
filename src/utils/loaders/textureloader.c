//
//  textureloader.c
//  C-ray
//
//  Created by Valtteri Koskivuori on 02/04/2019.
//  Copyright © 2015-2020 Valtteri Koskivuori. All rights reserved.
//

#include "../../includes.h"
#include "textureloader.h"
#include "../../utils/filehandler.h"
#include "../../utils/logging.h"
#include "../../datatypes/image/texture.h"
#include "../../datatypes/image/hdr.h"
#include "../../datatypes/color.h"

#define STBI_NO_PSD
#define STBI_NO_GIF
#define STB_IMAGE_IMPLEMENTATION
#include "../../libraries/stb_image.h"

//This is to compensate for the non-standard coordinate system handedness
struct texture *flipHorizontal(struct texture *t) {
	struct texture *new = newTexture(t->precision, t->width, t->height, t->channels);
	new->colorspace = t->colorspace;
	
	for (unsigned y = 0; y < new->height; ++y) {
		for (unsigned x = 0; x < new->width; ++x) {
			blit(new, textureGetPixel(t, ((t->width-1) - x), y), x, y);
		}
	}
	
	free(t);
	return new;
}

struct hdr *loadHDRI(char *filePath) {
	struct hdr *new = newHDRI();
	//Handle the trailing newline here
	//FIXME: This crashes if there is no newline, even though SO said it shouldn't.
	filePath[strcspn(filePath, "\n")] = 0;
	if (stbi_is_hdr(filePath)) {
		logr(info, "Loading HDR...");
		new->t->data.float_p = stbi_loadf(filePath, (int*)&new->t->width, (int*)&new->t->height, &new->t->channels, 0);
		new->t->precision = float_p;
		if (!new->t->data.float_p) {
			destroyHDRI(new);
			logr(warning, "Error while loading HDR from %s - Does the file exist?\n");
			return NULL;
		}
		float MB = (((getFileSize(filePath))/1000.0f)/1000.0f);
		printf(" %.1fMB\n", MB);
	} else {
		destroyHDRI(new);
	}
	return new;
}

struct texture *loadTexture(char *filePath) {
	struct texture *new = newTexture(none, 0, 0, 0);
	//Handle the trailing newline here
	//FIXME: This crashes if there is no newline, even though SO said it shouldn't.
	filePath[strcspn(filePath, "\n")] = 0;
	
	new->data.byte_p = stbi_load(filePath, (int*)&new->width, (int*)&new->height, &new->channels, 3);
	if (!new->data.byte_p) {
		logr(warning, "Error while loading texture from \"%s\" - Does the file exist?\n", filePath);
		destroyTexture(new);
		return NULL;
	}
	//new = flipHorizontal(new);
	new->precision = char_p;
	
	return new;
}
