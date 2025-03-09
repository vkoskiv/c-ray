//
//  encoder.c
//  c-ray
//
//  Created by Valtteri on 8.4.2020.
//  Copyright © 2020-2025 Valtteri Koskivuori. All rights reserved.
//

#include "encoder.h"

#include <imagefile.h>
#include <common/logging.h>
#include <common/cr_assert.h>
#include <common/texture.h>

#include "formats/png.h"
#include "formats/bmp.h"
#include "formats/qoi.h"
#include <stdio.h>
#include <c-ray/c-ray.h>

void writeImage(struct imageFile *image) {
	char *suffix;
	switch (image->type) {
		case bmp:
			suffix = "bmp";
			break;
		case png:
			suffix = "png";
			break;
		case qoi:
			suffix = "qoi";
			break;
		case hdr:
		case obj:
		case mtl:
		case jpg:
		case tiff:
		case gltf:
		case glb:
		default:
			logr(warning, "Unsupported file type, falling back to PNG\n");
			suffix = "png";
			image->type = png;
			break;
	}
	char buf[2048];
	snprintf(buf, 2048 - 1, "%s%s_%04d.%s", image->filePath, image->fileName, image->count, suffix);
	struct texture *tmp = newTexture(char_p, image->t->width, image->t->height, 3);
	textureToSRGB((struct texture *)image->t);
	for (size_t y = 0; y < tmp->height; ++y) {
		for (size_t x = 0; x < tmp->width; ++x) {
			setPixel(tmp, textureGetPixel((struct texture *)image->t, x, y, false), x, y);
		}
	}
	switch (image->type) {
		case png:
			encodePNGFromArray(buf, tmp->data.byte_p, tmp->width, tmp->height, image->info);
			break;
		case bmp:
			encodeBMPFromArray(buf, tmp->data.byte_p, tmp->width, tmp->height);
			break;
		case qoi:
			encode_qoi_from_array(buf, tmp->data.byte_p, tmp->width, tmp->height);
			break;
		case unknown:
		default:
			ASSERT_NOT_REACHED();
			break;
	}
	destroyTexture(tmp);
}
