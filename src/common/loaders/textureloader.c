//
//  textureloader.c
//  c-ray
//
//  Created by Valtteri Koskivuori on 02/04/2019.
//  Copyright © 2019-2025 Valtteri Koskivuori. All rights reserved.
//

#include "textureloader.h"
#include <logging.h>
#include <texture.h>

#define STBI_NO_PSD
#define STBI_NO_GIF
#define STB_IMAGE_IMPLEMENTATION
#include "../../common/vendored/stb_image.h"

#define QOI_IMPLEMENTATION
#define QOI_NO_STDIO
#include "../../common/vendored/qoi.h"

static int load_env_map(const file_data data, struct texture *out) {
	if (!out) return 1;
	float *new = stbi_loadf_from_memory(data.items, (int)data.count, (int *)&out->width, (int *)&out->height, (int *)&out->channels, 0);
	if (!new) {
		logr(warning, "Error while decoding HDR: %s\n", stbi_failure_reason());
		return 1;
	}
	if (out->data.byte_p) free(out->data.byte_p);
	out->data.float_p = new;
	out->precision = float_p;
	return 0;
}

static int load_qoi_from_buffer(const file_data data, struct texture *out) {
	if (!out) return 1;
	qoi_desc desc;
	void *decoded_data = qoi_decode(data.items, data.count, &desc, 3);
	if (!decoded_data) {
		logr(warning, "Error while decoding QOI\n");
		return 1;
	}
	if (out->data.byte_p) free(out->data.byte_p);
	out->data.byte_p = decoded_data;
	out->width = desc.width;
	out->height = desc.height;
	out->channels = desc.channels;
	out->precision = char_p;
	// TODO: Maybe don't assume
	out->colorspace = sRGB;
	return 0;
}

static int load_texture_from_buffer(const file_data data, struct texture *out) {
	if (!out) return 1;
	unsigned char *new = stbi_load_from_memory(data.items, data.count, (int *)&out->width, (int *)&out->height, (int *)&out->channels, 0);
	if (!new) {
		logr(warning, "Failed to decode texture from memory buffer of size %zu. Reason: \"%s\"\n", data.count, stbi_failure_reason());
		return 1;
	}
	if (out->data.byte_p) free(out->data.byte_p);
	out->data.byte_p = new;
	out->precision = char_p;
	// TODO: Maybe don't assume
	out->colorspace = sRGB;
	return 0;
}

int load_texture(const char *path, file_data data, struct texture *out) {
	if (!path || !data.items || !out) return 1;

	enum fileType type = guess_file_type(path);
	int ret = 0;
	if (stbi_is_hdr_from_memory(data.items, data.count)) {
		ret = load_env_map(data, out);
		if (ret) goto error;
	} else if (type == qoi) {
		ret = load_qoi_from_buffer(data, out);
		if (ret) goto error;
	} else {
		ret = load_texture_from_buffer(data, out);
		if (ret) goto error;
	}

	size_t raw_bytes = (out->channels * (out->precision == float_p ? 4 : 1)) * out->width * out->height;
	char b0[64];
	char b1[64];
	logr(debug, "Loaded texture %s, %s => %s\n", path, human_file_size(data.count, b0), human_file_size(raw_bytes, b1));
	return 0;
error:
	logr(warning, "^That happened while decoding texture \"%s\"\n", path);
	return 1;
}
