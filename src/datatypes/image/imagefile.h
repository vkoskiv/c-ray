//
//  imagefile.h
//  C-ray
//
//  Created by Valtteri on 16.4.2020.
//  Copyright © 2020 Valtteri Koskivuori. All rights reserved.
//

#pragma once

enum fileType {
	bmp,
	png,
	hdr,
};

struct imageFile {
	struct texture *t;
	enum fileType type;
	char *filePath;
	char *fileName;
	int count;
};

struct imageFile *newImageFile(struct texture *t, char *filePath, char *fileName, int count, enum fileType type);
