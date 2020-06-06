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

struct renderInfo {
	int samples;
	int bounces;
	unsigned long long renderTime;
	int threadCount;
	char *arch;
	char *crayVersion;
	char *gitHash;
};

struct imageFile {
	struct texture *t;
	enum fileType type;
	char *filePath;
	char *fileName;
	int count;
	struct renderInfo info;
};

struct imageFile *newImageFile(struct texture *t, char *filePath, char *fileName, int count, enum fileType type);

void destroyImageFile(struct imageFile *file);
