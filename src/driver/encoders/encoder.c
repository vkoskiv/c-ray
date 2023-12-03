//
//  encoder.c
//  C-ray
//
//  Created by Valtteri on 8.4.2020.
//  Copyright © 2020-2021 Valtteri Koskivuori. All rights reserved.
//

#include "encoder.h"

#include "../imagefile.h"
#include "../../common/logging.h"
#include "../../common/texture.h"
#include "../../common/assert.h"

#include "formats/png.h"
#include "formats/bmp.h"
#include "formats/qoi.h"
#include <stdio.h>

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
	switch (image->type) {
		case png:
			encodePNGFromArray(buf, image->t->data.byte_p, image->t->width, image->t->height, image->info);
			break;
		case bmp:
			encodeBMPFromArray(buf, image->t->data.byte_p, image->t->width, image->t->height);
			break;
		case qoi:
			encode_qoi_from_array(buf, image->t->data.byte_p, image->t->width, image->t->height);
			break;
		case unknown:
		default:
			ASSERT_NOT_REACHED();
			break;
	}
}
