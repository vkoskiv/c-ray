//
//  bmp.c
//  C-ray
//
//  Created by Valtteri on 8.4.2020.
//  Copyright © 2020-2022 Valtteri Koskivuori. All rights reserved.
//

#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include "bmp.h"

#include "../../../utils/fileio.h"

void encodeBMPFromArray(const char *file_name, const unsigned char *imgData, size_t width, size_t height) {
	//Apparently BMP is BGR, whereas C-ray's internal buffer is RGB (Like it should be)
	//So we need to convert the image data before writing to file.
	unsigned char *bgr_data = malloc(3 * width * height);
	for (unsigned y = 0; y < height; ++y) {
		for (unsigned x = 0; x < width; ++x) {
			size_t offset = (x + (height - (y + 1)) * width) * 3;
			bgr_data[offset + 0] = imgData[offset + 2];
			bgr_data[offset + 1] = imgData[offset + 1];
			bgr_data[offset + 2] = imgData[offset + 0];
		}
	}

	unsigned char bmp_file_header[14] = { 'B', 'M', 0, 0, 0, 0, 0, 0, 0, 0, 54, 0, 0, 0 };
	unsigned char bmp_info_header[40] = { 40, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 24, 0 };
	unsigned char bmp_padding[3] = { 0, 0, 0 };
	size_t file_size = sizeof(bmp_file_header) + sizeof(bmp_info_header) + sizeof(bmp_padding) * width * height;
	//Create header with file_size data
	bmp_file_header[2] = (unsigned char)(file_size      );
	bmp_file_header[3] = (unsigned char)(file_size >> 8);
	bmp_file_header[4] = (unsigned char)(file_size >> 16);
	bmp_file_header[5] = (unsigned char)(file_size >> 24);
	//create header width and height info
	bmp_info_header[ 4] = (unsigned char)(width     );
	bmp_info_header[ 5] = (unsigned char)(width >> 8);
	bmp_info_header[ 6] = (unsigned char)(width >> 16);
	bmp_info_header[ 7] = (unsigned char)(width >> 24);
	bmp_info_header[ 8] = (unsigned char)(height      );
	bmp_info_header[ 9] = (unsigned char)(height >> 8);
	bmp_info_header[10] = (unsigned char)(height >> 16);
	bmp_info_header[11] = (unsigned char)(height >> 24);

	size_t offset = 0;
	unsigned char *file_contents = malloc(file_size);

	memcpy(file_contents, bmp_file_header, sizeof(bmp_file_header));
	offset += sizeof(bmp_file_header);
	memcpy(file_contents + offset, bmp_info_header, sizeof(bmp_info_header));
	offset += sizeof(bmp_info_header);
	for (unsigned i = 1; i <= height; ++i) {
		memcpy(file_contents + offset, bgr_data + (width * (height - i) * 3), 3 * width);
		offset += 3 * width;
		size_t padding_bytes_to_write = (4 - (width * 3) % 4) % 4;
		memcpy(file_contents + offset, bmp_padding, padding_bytes_to_write);
		offset += padding_bytes_to_write;
	}
	free(bgr_data);
	write_file((file_data){ .items = file_contents, .count = file_size }, file_name);
	free(file_contents);
}
