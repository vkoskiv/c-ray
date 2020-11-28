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

#include "formats/png.h"
#include "formats/bmp.h"

void writeImage(struct imageFile *image) {
	char *buf = NULL;
	if (asprintf(&buf, "%s%s_%04d.%s", image->filePath, image->fileName, image->count, image->type == png ? "png" : "bmp") < 0) {
		logr(error, "asprintf failed in writeImage for file %s\n", image->fileName);
	}
	switch (image->type) {
		case png:
			encodePNGFromArray(buf, image->t->data.byte_p, image->t->width, image->t->height, image->info);
			break;
		case bmp:
			encodeBMPFromArray(buf, image->t->data.byte_p, image->t->width, image->t->height);
			break;
		default:
			ASSERT_NOT_REACHED();
			break;
	}
	free(buf);
}
