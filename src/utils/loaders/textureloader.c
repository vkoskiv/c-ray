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
#include "../../datatypes/texture.h"
#include "../../datatypes/color.h"
#include "../../utils/string.h"

#define STBI_NO_PSD
#define STBI_NO_GIF
#define STB_IMAGE_IMPLEMENTATION
#include "../../libraries/stb_image.h"

//This is to compensate for the non-standard coordinate system handedness
struct texture *flipHorizontal(struct texture *t) {
	struct texture *new = newTexture(t->precision, t->width, t->height, t->channels);
	new->colorspace = t->colorspace;
	new->count = t->count;
	if (t->fileName) {
		copyString(t->fileName, &new->fileName);
	}
	if (t->filePath) {
		copyString(t->filePath, &new->filePath);
	}
	new->fileType = t->fileType;
	
	for (unsigned y = 0; y < new->height; ++y) {
		for (unsigned x = 0; x < new->width; ++x) {
			blit(new, textureGetPixel(t, ((t->width-1) - x), y), x, y);
		}
	}
	
	free(t);
	return new;
}

struct texture *loadTexture(char *filePath) {
	struct texture *new = newTexture(none, 0, 0, 0);
	copyString(filePath, &new->filePath);
	//Handle the trailing newline here
	//FIXME: This crashes if there is no newline, even though SO said it shouldn't.
	filePath[strcspn(filePath, "\n")] = 0;
	
	if (stbi_is_hdr(filePath)) {
		logr(info, "Loading HDR...");
		new->fileType = hdr;
		new->float_data = stbi_loadf(filePath, (int*)&new->width, (int*)&new->height, &new->channels, 0);
		new->precision = float_p;
		if (!new->float_data) {
			destroyTexture(new);
			logr(warning, "Error while loading HDR from %s - Does the file exist?\n");
			return NULL;
		}
		float MB = (((getFileSize(filePath))/1000.0)/1000.0);
		printf(" %.1fMB\n", MB);
	} else {
		new->byte_data = stbi_load(filePath, (int*)&new->width, (int*)&new->height, &new->channels, 3);
		if (!new->byte_data) {
			logr(warning, "Error while loading texture from \"%s\" - Does the file exist?\n", filePath);
			destroyTexture(new);
			return NULL;
		}
		new->fileType = buffer;
		//new = flipHorizontal(new);
		new->precision = char_p;
	}
	
	return new;
}
