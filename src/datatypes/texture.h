//
//  texture.h
//  C-ray
//
//  Created by Valtteri on 09/04/2019.
//  Copyright © 2015-2019 Valtteri Koskivuori. All rights reserved.
//

enum fileType {
	bmp,
	png
};

struct dimensions {
	int height;
	int width;
};

struct texture {
	enum fileType fileType;
	char *filePath;
	char *fileName;
	int count;
	unsigned char *data;
	unsigned int *width;
	unsigned int *height;
};
