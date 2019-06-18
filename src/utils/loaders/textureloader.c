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

//This is a bit of a hack, my coordinate space is inverted.
struct texture *flipHorizontal(struct texture *t) {
	struct texture *newTex = calloc(1, sizeof(struct texture));
	newTex->data = calloc(3 * *t->width * *t->height, sizeof(unsigned char));
	newTex->count = t->colorspace;
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
struct texture *loadTexture(char *filePath) {
	struct texture *newTexture = calloc(1, sizeof(struct texture));
	newTexture->data = NULL;
	newTexture->colorspace = linear;
	newTexture->width = calloc(1, sizeof(unsigned int));
	newTexture->height = calloc(1, sizeof(unsigned int));
	newTexture->hasAlpha = true;
	copyString(filePath, &newTexture->filePath);
	newTexture->fileType = png;
	
	//Handle the trailing newline here
	//FIXME: This crashes if there is no newline, even though SO said it shouldn't.
	filePath[strcspn(filePath, "\n")] = 0;
	int err = lodepng_decode32_file(&newTexture->data, newTexture->width, newTexture->height, filePath);
	if (err != 0) {
		logr(warning, "Texture loading error at %s: %s\n", filePath, lodepng_error_text(err));
		free(newTexture);
		return NULL;
	}
	//textureFromSRGB(newTexture);
	newTexture = flipHorizontal(newTexture);
	return newTexture;
}
