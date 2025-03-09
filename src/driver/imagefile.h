//
//  imagefile.h
//  c-ray
//
//  Created by Valtteri on 16.4.2020.
//  Copyright © 2020-2025 Valtteri Koskivuori. All rights reserved.
//

#pragma once

#include <common/fileio.h>

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
	struct cr_bitmap *t;
	enum fileType type;
	const char *filePath;
	const char *fileName;
	int count;
	struct renderInfo info;
};
