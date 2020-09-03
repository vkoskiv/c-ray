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
#include "../ioutils.h"
#include "../../libraries/asprintf.h"

#include "formats/png.h"
#include "formats/bmp.h"

void writeImage(struct imageFile *image) {
	//Save image data to a file
	char *buf = NULL;
	if (image->type == bmp){
		asprintf(&buf, "%s%s_%04d.bmp", image->filePath, image->fileName, image->count);
		encodeBMPFromArray(buf, image->t->data.byte_p, image->t->width, image->t->height);
	} else if (image->type == png){
		asprintf(&buf, "%s%s_%04d.png", image->filePath, image->fileName, image->count);
		encodePNGFromArray(buf, image->t->data.byte_p, image->t->width, image->t->height, image->info);
	}
	free(buf);
}
