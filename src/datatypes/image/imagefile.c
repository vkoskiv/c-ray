//
//  imagefile.c
//  C-ray
//
//  Created by Valtteri on 16.4.2020.
//  Copyright © 2020 Valtteri Koskivuori. All rights reserved.
//

#include "imagefile.h"
#include "texture.h"
#include <stdlib.h>
#include "../../utils/string.h"

struct imageFile *newImageFile(struct texture *t, char *filePath, char *fileName, int count, enum fileType type) {
	struct imageFile *file = calloc(1, sizeof(*file));
	file->t = t;
	file->filePath = copyString(filePath);
	file->fileName = copyString(fileName);
	file->count = count;
	file->type = type;
	return file;
}

void destroyImageFile(struct imageFile *file) {
	if (file) {
		if (file->t) destroyTexture(file->t);
		if (file->fileName) free(file->fileName);
		if (file->filePath) free(file->filePath);
		free(file);
	}
}
