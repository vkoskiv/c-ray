//
//  encoder.c
//  C-ray
//
//  Created by Valtteri on 8.4.2020.
//  Copyright © 2020 Valtteri Koskivuori. All rights reserved.
//

#include "encoder.h"

#include "../../includes.h"
#include "../../datatypes/image/texture.h"
#include "../../datatypes/image/imagefile.h"
#include "../logging.h"
#include "../fileio.h"
#include "../../libraries/asprintf.h"
#include "../assert.h"
#include "../args.h"
#include "../string.h"

#include "formats/png.h"
#include "formats/bmp.h"

void writeImage(struct imageFile *image) {
	char *buf = NULL;
	if (isSet("output_path")) {
		asprintf(&buf, "%s%s", image->filePath, image->fileName);
		if (stringEndsWith(".png", buf)) {
			image->type = png;
		} else if (stringEndsWith(".bmp", buf)) {
			image->type = bmp;
		} else {
			image->type = unknown;
		}
	} else {
		asprintf(&buf, "%s%s_%04d.%s", image->filePath, image->fileName, image->count, image->type == png ? "png" : "bmp");
	}
	switch (image->type) {
		case png:
			encodePNGFromArray(buf, image->t->data.byte_p, image->t->width, image->t->height, image->info);
			break;
		case bmp:
			encodeBMPFromArray(buf, image->t->data.byte_p, image->t->width, image->t->height);
			break;
		case unknown:
			logr(warning, "Unknown file type with -o flag, defaulting to png\n");
			encodePNGFromArray(buf, image->t->data.byte_p, image->t->width, image->t->height, image->info);
			break;
		default:
			ASSERT_NOT_REACHED();
			break;
	}
	free(buf);
}
