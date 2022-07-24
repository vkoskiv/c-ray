//
//  textureloader.c
//  C-ray
//
//  Created by Valtteri Koskivuori on 02/04/2019.
//  Copyright © 2019-2022 Valtteri Koskivuori. All rights reserved.
//

#include "textureloader.h"
#include "../fileio.h"
#include "../logging.h"
#include "../../datatypes/image/texture.h"
#include "../../utils/assert.h"
#include "../../utils/mempool.h"

#define STBI_NO_PSD
#define STBI_NO_GIF
#define STB_IMAGE_IMPLEMENTATION
#include "../../vendored/stb_image.h"
#include "../../vendored/qoi.h" // encoder defines implementation macro already

// I don't want to mess with memory allocation within the different
// image parsing libs, so I just copy out to a pool afterwards.
void copy_to_pool(struct block **pool, struct texture *tex) {
	size_t unitSize = tex->precision == float_p ? sizeof(float) : sizeof(unsigned char);
	size_t bytes = tex->width * tex->height * tex->channels * unitSize;
	void *newBuf = allocBlock(pool, bytes);
	memcpy(newBuf, tex->data.byte_p, bytes);
	free(tex->data.byte_p);
	tex->data.byte_p = newBuf;
}

static struct texture *load_env_map(unsigned char *buf, size_t buflen, const char *path, struct block **pool) {
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

// We use copyToPool() in loadTexture to copy the actual image data into the memory pool. This code is a bit confusing.
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

struct texture *load_texture_from_buffer(const unsigned char *buffer, const unsigned int buflen, struct block **pool) {
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

struct texture *load_texture(char *filePath, struct block **pool, struct file_cache *cache) {
	size_t len = 0;
	//Handle the trailing newline here
	filePath[strcspn(filePath, "\n")] = 0;
	unsigned char *file = (unsigned char*)loadFile(filePath, &len, cache);
	if (!file) return NULL;
	
	enum fileType type = guessFileType(filePath);
	
	struct texture *new = NULL;
	if (stbi_is_hdr(filePath)) {
		new = load_env_map(file, len, filePath, pool);
	} else if (type == qoi) {
		new = load_qoi_from_buffer(file, (unsigned int)len, pool);
	} else {
		new = load_texture_from_buffer(file, (unsigned int)len, pool);
	}
	free(file);
	if (pool) copy_to_pool(pool, new);
	if (!new) {
		logr(warning, "^That happened while decoding texture \"%s\" - Corrupted?\n", filePath);
		destroyTexture(new);
		return NULL;
	}
	
	return new;
}
