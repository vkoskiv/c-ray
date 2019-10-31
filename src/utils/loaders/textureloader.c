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

<<<<<<< HEAD
//This is a bit of a hack, my vec2inate space is inverted.
=======
//This is a bit of a hack, my coordinate space is inverted.
//TODO: Just flip the buffer instead
>>>>>>> 1d60640fe22419135cd05015879227d4992e474f
struct texture *flipHorizontal(struct texture *t) {
	struct texture *new = newTexture();
	allocTextureBuffer(new, t->precision, *t->width, *t->height, *t->channels);
	new->colorspace = t->colorspace;
	new->count = t->count;
	if (t->fileName) {
		copyString(t->fileName, &new->fileName);
	}
	if (t->filePath) {
		copyString(t->filePath, &new->filePath);
	}
	new->fileType = t->fileType;
	
	for (int y = 0; y < *new->height; y++) {
		for (int x = 0; x < *new->width; x++) {
			blit(new, textureGetPixel(t, ((*t->width-1) - x), y), x, y);
		}
	}
	
	free(t);
	return new;
}

//TODO: Detect and support other file formats, like TIFF, JPEG and BMP
//Currently supports: PNG, HDR
struct texture *loadTexture(char *filePath) {
	struct texture *new = newTexture();
	copyString(filePath, &new->filePath);
	//Handle the trailing newline here
	//FIXME: This crashes if there is no newline, even though SO said it shouldn't.
	filePath[strcspn(filePath, "\n")] = 0;
	//FIXME: Ignoring alpha here, and set hasAlpha to false before for now
	
	if (stbi_is_hdr(filePath)) {
		new->fileType = hdr;
		new->float_data = stbi_loadf(filePath, (int*)new->width, (int*)new->height, new->channels, 0);
		new->precision = float_p;
		if (!new->float_data) {
			freeTexture(new);
			free(new);
			logr(warning, "Error while loading HDR from %s - Does the file exist?\n");
			return NULL;
		} else {
			int MB = (((*new->width * *new->height * sizeof(float))/1024)/1024);
			logr(info, "Loaded %iMB Radiance file with pitch %i\n", MB, *new->channels);
		}
	} else {
		int err = (int)lodepng_decode24_file(&new->byte_data, new->width, new->height, filePath);
		if (err != 0) {
			logr(warning, "Texture loading error at %s: %s\n", filePath, lodepng_error_text(err));
			freeTexture(new);
			free(new);
			return NULL;
		}
		*new->channels = 3;
		new->fileType = png;
		new = flipHorizontal(new);
		new->precision = char_p;
	}
	
	return new;
}
