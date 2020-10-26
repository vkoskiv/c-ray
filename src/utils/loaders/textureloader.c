//
//  textureloader.c
//  C-ray
//
//  Created by Valtteri Koskivuori on 02/04/2019.
//  Copyright © 2015-2020 Valtteri Koskivuori. All rights reserved.
//

#include "../../includes.h"
#include "textureloader.h"
#include "../fileio.h"
#include "../logging.h"
#include "../../datatypes/image/texture.h"
#include "../../datatypes/image/hdr.h"
#include "../../datatypes/color.h"

#define STBI_NO_PSD
#define STBI_NO_GIF
#define STB_IMAGE_IMPLEMENTATION
#include "../../libraries/stb_image.h"

struct hdr *loadHDRI(char *filePath) {
	size_t len = 0;
	//Handle the trailing newline here
	//FIXME: This crashes if there is no newline, even though SO said it shouldn't.
	filePath[strcspn(filePath, "\n")] = 0;
	const unsigned char *file = (unsigned char*)loadFile(filePath, &len);
	if (!file) return NULL;
	struct hdr *new = newHDRI();
	if (stbi_is_hdr(filePath)) {
		logr(info, "Loading HDR...");
		new->t->data.float_p = stbi_loadf_from_memory(file, (int)len, (int*)&new->t->width, (int*)&new->t->height, &new->t->channels, 0);
		new->t->precision = float_p;
		if (!new->t->data.float_p) {
			destroyHDRI(new);
			new = NULL;
			logr(warning, "Error while decoding HDR from %s - Corrupted?\n", filePath);
			return NULL;
		}
		float MB = (((getFileSize(filePath))/1000.0f)/1000.0f);
		printf(" %.1fMB\n", MB);
	} else {
		destroyHDRI(new);
		new = NULL;
	}
	return new;
}

struct texture *loadTexture(char *filePath) {
	size_t len = 0;
	//Handle the trailing newline here
	//FIXME: This crashes if there is no newline, even though SO said it shouldn't.
	filePath[strcspn(filePath, "\n")] = 0;
	const unsigned char *file = (unsigned char*)loadFile(filePath, &len);
	if (!file) return NULL;
	struct texture *new = loadTextureFromBuffer(file, (unsigned int)len, 4);
	if (!new) {
		logr(warning, "^That happened while decoding texture \"%s\" - Corrupted?\n", filePath);
		destroyTexture(new);
		return NULL;
	}
	return new;
}

struct texture *loadTextureFromBuffer(const unsigned char *buffer, const unsigned int buflen, int reqComp) {
	struct texture *new = newTexture(none, 0, 0, 0);
	new->data.byte_p = stbi_load_from_memory(buffer, buflen, (int*)&new->width, (int*)&new->height, &new->channels, reqComp);
	if (!new->data.byte_p) {
		logr(warning, "Failed to decode texture from memory buffer of size %u", buflen);
		destroyTexture(new);
		return NULL;
	}
	if (new->channels > 3) {
		new->hasAlpha = true;
	}
	new->precision = char_p;
	return new;
}
