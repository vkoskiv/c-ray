//
//  encoder.c
//  C-ray
//
//  Created by Valtteri on 8.4.2020.
//  Copyright © 2020-2021 Valtteri Koskivuori. All rights reserved.
//

#include "encoder.h"

#include "../../datatypes/image/texture.h"
#include "../../datatypes/image/imagefile.h"
#include "../logging.h"
#include "../../vendored/asprintf.h"
#include "../assert.h"
#include "../args.h"

#include "formats/png.h"
#include "formats/bmp.h"
#include "formats/qoi.h"

void writeImage(struct imageFile *image) {
	char *buf = NULL;
	if (isSet("output_path")) {
		asprintf(&buf, "%s%s", image->filePath, image->fileName);
		image->type = guessFileType(buf);
	} else {
		asprintf(&buf, "%s%s_%04d.%s", image->filePath, image->fileName, image->count, image->type == png ? "png" : image->type == bmp ? "bmp" : "qoi");
	}
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
			logr(warning, "Unknown file type with -o flag, defaulting to png\n");
			encodePNGFromArray(buf, image->t->data.byte_p, image->t->width, image->t->height, image->info);
			break;
		default:
			ASSERT_NOT_REACHED();
			break;
	}
	free(buf);
}
