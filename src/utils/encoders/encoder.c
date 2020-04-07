//
//  encoder.c
//  C-ray
//
//  Created by Valtteri on 8.4.2020.
//  Copyright © 2020 Valtteri Koskivuori. All rights reserved.
//

#include "encoder.h"

#include "../../includes.h"
#include "../../datatypes/texture.h"
#include "../logging.h"
#include "../filehandler.h"
#include "../../libraries/asprintf.h"

#include "formats/png.h"
#include "formats/bmp.h"

void writeImage(struct texture *image, struct renderInfo imginfo) {
	//Save image data to a file
	char *buf = NULL;
	if (image->fileType == bmp){
		asprintf(&buf, "%s%s_%04d.bmp", image->filePath, image->fileName, image->count);
		encodeBMPFromArray(buf, image->byte_data, image->width, image->height);
	} else if (image->fileType == png){
		asprintf(&buf, "%s%s_%04d.png", image->filePath, image->fileName, image->count);
		encodePNGFromArray(buf, image->byte_data, image->width, image->height, imginfo);
	}
	logr(info, "Saving result in \"%s\"\n", buf);
	printFileSize(buf);
	free(buf);
}
