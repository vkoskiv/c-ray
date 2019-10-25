//
//  textureloader.c
//  C-ray
//
//  Created by Valtteri Koskivuori on 02/04/2019.
//  Copyright © 2015-2019 Valtteri Koskivuori. All rights reserved.
//

#include "../../includes.h"
#include "textureloader.h"
#include "../../utils/filehandler.h"
#include "../../utils/logging.h"

#include "../../datatypes/texture.h"

#define STBI_ONLY_HDR
#define STB_IMAGE_IMPLEMENTATION
#include "../../libraries/stb_image.h"

//This is a bit of a hack, my vec2inate space is inverted.
struct texture *flipHorizontal(struct texture *t) {
	struct texture *newTex = calloc(1, sizeof(struct texture));
	newTex->byte_data = calloc(3 * *t->width * *t->height, sizeof(unsigned char));
	newTex->colorspace = t->colorspace;
	newTex->count = t->count;
	newTex->width = calloc(1, sizeof(unsigned int));
	*newTex->width = *t->width;
	newTex->height = calloc(1, sizeof(unsigned int));
	*newTex->height = *t->height;
	if (t->fileName) {
		copyString(t->fileName, &newTex->fileName);
	}
	if (t->filePath) {
		copyString(t->filePath, &newTex->filePath);
	}
	newTex->fileType = t->fileType;
	
	for (int y = 0; y < *newTex->height; y++) {
		for (int x = 0; x < *newTex->width; x++) {
			blit(newTex, textureGetPixel(t, ((*t->width-1) - x), y), x, y);
		}
	}
	
	free(t);
	return newTex;
}

//TODO: Detect and support other file formats, like TIFF, JPEG and BMP
//Currently supports: PNG, HDR
struct texture *loadTexture(char *filePath) {
	struct texture *newTexture = calloc(1, sizeof(struct texture));
	newTexture->byte_data = NULL;
	newTexture->float_data = NULL;
	newTexture->channels = NULL;
	newTexture->colorspace = linear; //Assumed for now
	newTexture->width = calloc(1, sizeof(unsigned int));
	newTexture->height = calloc(1, sizeof(unsigned int));
	newTexture->hasAlpha = false;
	copyString(filePath, &newTexture->filePath);
	newTexture->fileType = png;
	
	//Handle the trailing newline here
	//FIXME: This crashes if there is no newline, even though SO said it shouldn't.
	filePath[strcspn(filePath, "\n")] = 0;
	//FIXME: Ignoring alpha here, and set hasAlpha to false before for now
	
	if (stbi_is_hdr(filePath)) {
		newTexture->fileType = hdr;
		newTexture->hasAlpha = false;
		newTexture->channels = calloc(1, sizeof(int));
		newTexture->float_data = stbi_loadf(filePath, (int*)newTexture->width, (int*)newTexture->height, newTexture->channels, 0);
		if (!newTexture->float_data) {
			free(newTexture);
			logr(warning, "Error while loading HDR from %s - Does the file exist?\n");
			return NULL;
		} else {
			int MB = (((*newTexture->width * *newTexture->height * sizeof(float))/1024)/1024);
			logr(info, "Loaded %iMB Radiance file with pitch %i\n", MB, *newTexture->channels);
		}
	} else {
		int err = (int)lodepng_decode24_file(&newTexture->byte_data, newTexture->width, newTexture->height, filePath);
		if (err != 0) {
			logr(warning, "Texture loading error at %s: %s\n", filePath, lodepng_error_text(err));
			free(newTexture);
			return NULL;
		}
		newTexture = flipHorizontal(newTexture);
	}
	
	return newTexture;
}
