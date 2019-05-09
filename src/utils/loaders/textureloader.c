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

//TODO: Detect and support other file formats, like TIFF, JPEG and BMP
struct texture *loadTexture(char *filePath) {
	struct texture *newTexture = calloc(1, sizeof(struct texture));
	newTexture->data = NULL;
	newTexture->width = calloc(1, sizeof(unsigned int));
	newTexture->height = calloc(1, sizeof(unsigned int));
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
	return newTexture;
}

void freeTexture(struct texture *tex) {
	if (tex->height) {
		free(tex->height);
	}
	if (tex->width) {
		free(tex->width);
	}
	if (tex->data) {
		free(tex->data);
	}
}
