//
//  textureloader.c
//  C-ray
//
//  Created by Valtteri Koskivuori on 02/04/2019.
//  Copyright © 2019-2020 Valtteri Koskivuori. All rights reserved.
//

#include "../../includes.h"
#include "textureloader.h"
#include "../fileio.h"
#include "../logging.h"
#include "../../datatypes/image/texture.h"
#include "../../datatypes/color.h"
#include "../../utils/assert.h"
#include "../../utils/mempool.h"

#define STBI_NO_PSD
#define STBI_NO_GIF
#define STB_IMAGE_IMPLEMENTATION
#include "../../libraries/stb_image.h"
#include "../../libraries/qoi.h" // encoder defines implementation macro already

struct texture *load_qoi_from_buffer(const unsigned char *buffer, const unsigned int buflen, struct block **pool);

// I don't want to mess with memory allocation within the different
// image parsing libs, so I just copy out to a pool afterwards.
void copyToPool(struct block **pool, struct texture *tex) {
	size_t unitSize = tex->precision == float_p ? sizeof(float) : sizeof(unsigned char);
	size_t bytes = tex->width * tex->height * tex->channels * unitSize;
	void *newBuf = allocBlock(pool, bytes);
	memcpy(newBuf, tex->data.byte_p, bytes);
	free(tex->data.byte_p);
	tex->data.byte_p = newBuf;
}

static struct texture *loadEnvMap(unsigned char *buf, size_t buflen, const char *path, struct block **pool) {
	ASSERT(buf);
	logr(info, "Loading HDR...");
	struct texture *tex = allocBlock(pool, sizeof(*tex));
	tex->data.float_p = stbi_loadf_from_memory(buf, (int)buflen, (int *)&tex->width, (int *)&tex->height, (int *)&tex->channels, 0);
	tex->precision = float_p;
	if (!tex->data.float_p) {
		destroyTexture(tex);
		logr(warning, "Error while decoding HDR from %s - Corrupted?\n", path);
		return NULL;
	}
	char *fsbuf = humanFileSize(buflen);
	printf(" %s\n", fsbuf);
	free(fsbuf);
	return tex;
}

struct texture *loadTexture(char *filePath, struct block **pool) {
	size_t len = 0;
	//Handle the trailing newline here
	filePath[strcspn(filePath, "\n")] = 0;
	unsigned char *file = (unsigned char*)loadFile(filePath, &len);
	if (!file) return NULL;
	
	enum fileType type = guessFileType(filePath);
	
	struct texture *new = NULL;
	if (stbi_is_hdr(filePath)) {
		new = loadEnvMap(file, len, filePath, pool);
	} else if (type == qoi) {
		new = load_qoi_from_buffer(file, (unsigned int)len, pool);
	} else {
		new = loadTextureFromBuffer(file, (unsigned int)len, pool);
	}
	free(file);
	if (pool) copyToPool(pool, new);
	if (!new) {
		logr(warning, "^That happened while decoding texture \"%s\" - Corrupted?\n", filePath);
		destroyTexture(new);
		return NULL;
	}
	
	return new;
}

//FIXME: These don't actually put the image data in the memory pool. That'll leak!
struct texture *load_qoi_from_buffer(const unsigned char *buffer, const unsigned int buflen, struct block **pool) {
	qoi_desc desc;
	void *decoded_data = qoi_decode(buffer, buflen, &desc, 3);
	struct texture *new = pool ? allocBlock(pool, sizeof(*new)) : newTexture(none, 0, 0, 0);
	new->data.byte_p = decoded_data;
	new->width = desc.width;
	new->height = desc.height;
	new->channels = desc.channels;
	if (new->channels > 3) new->hasAlpha = true;
	new->precision = char_p;
	return new;
}

struct texture *loadTextureFromBuffer(const unsigned char *buffer, const unsigned int buflen, struct block **pool) {
	struct texture *new = pool ? allocBlock(pool, sizeof(*new)) : newTexture(none, 0, 0, 0);
	new->data.byte_p = stbi_load_from_memory(buffer, buflen, (int *)&new->width, (int *)&new->height, (int *)&new->channels, 0);
	if (!new->data.byte_p) {
		logr(warning, "Failed to decode texture from memory buffer of size %u. Reason: \"%s\"\n", buflen, stbi_failure_reason());
		destroyTexture(new);
		return NULL;
	}
	if (new->channels > 3) {
		new->hasAlpha = true;
	}
	new->precision = char_p;
	return new;
}
