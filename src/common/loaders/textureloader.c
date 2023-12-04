//
//  textureloader.c
//  c-ray
//
//  Created by Valtteri Koskivuori on 02/04/2019.
//  Copyright © 2019-2023 Valtteri Koskivuori. All rights reserved.
//

#include "textureloader.h"
#include "../logging.h"
#include "../texture.h"
#include "../assert.h"
#include "../mempool.h"

#define STBI_NO_PSD
#define STBI_NO_GIF
#define STB_IMAGE_IMPLEMENTATION
#include "../../common/vendored/stb_image.h"
#include "../../common/vendored/qoi.h" // encoder defines implementation macro already

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

static struct texture *load_env_map(const file_data data) {
	logr(info, "Loading HDR...");
	struct texture *tex = newTexture(none, 0, 0, 0);
	tex->data.float_p = stbi_loadf_from_memory(data.items, (int)data.count, (int *)&tex->width, (int *)&tex->height, (int *)&tex->channels, 0);
	tex->precision = float_p;
	if (!tex->data.float_p) {
		destroyTexture(tex);
		logr(warning, "Error while decoding HDR: %s\n", stbi_failure_reason());
		return NULL;
	}
	char sbuf[64];
	printf(" %s\n", human_file_size(data.count, sbuf));
	return tex;
}

struct texture *load_qoi_from_buffer(const file_data data) {
	qoi_desc desc;
	void *decoded_data = qoi_decode(data.items, data.count, &desc, 3);
	if (!decoded_data) return NULL;
	struct texture *new = newTexture(none, 0, 0, 0);
	new->data.byte_p = decoded_data;
	new->width = desc.width;
	new->height = desc.height;
	new->channels = desc.channels;
	new->precision = char_p;
	return new;
}

static struct texture *load_texture_from_buffer(const file_data data) {
	struct texture *new = newTexture(none, 0, 0, 0);
	new->data.byte_p = stbi_load_from_memory(data.items, data.count, (int *)&new->width, (int *)&new->height, (int *)&new->channels, 0);
	if (!new->data.byte_p) {
		logr(warning, "Failed to decode texture from memory buffer of size %zu. Reason: \"%s\"\n", data.count, stbi_failure_reason());
		destroyTexture(new);
		return NULL;
	}
	new->precision = char_p;
	return new;
}

struct texture *load_texture(const char *path, const file_data data) {
	if (!data.items) return NULL;

	enum fileType type = guess_file_type(path);

	struct texture *new = NULL;
	if (stbi_is_hdr_from_memory(data.items, data.count)) {
		new = load_env_map(data);
	} else if (type == qoi) {
		new = load_qoi_from_buffer(data);
	} else {
		new = load_texture_from_buffer(data);
	}

	if (!new) {
		logr(warning, "^That happened while decoding texture \"%s\"\n", path);
		return NULL;
	}

	size_t raw_bytes = (new->channels * (new->precision == float_p ? 4 : 1)) * new->width * new->height;
	char b0[64];
	char b1[64];
	logr(debug, "Loaded texture %s, %s => %s\n", path, human_file_size(data.count, b0), human_file_size(raw_bytes, b1));
	return new;
}
