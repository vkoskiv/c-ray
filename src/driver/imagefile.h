//
//  imagefile.h
//  C-ray
//
//  Created by Valtteri on 16.4.2020.
//  Copyright © 2020-2021 Valtteri Koskivuori. All rights reserved.
//

#pragma once

#include "../common/fileio.h"

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
	const char *filePath;
	const char *fileName;
	int count;
	struct renderInfo info;
};
