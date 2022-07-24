//
//  qoi.c
//  C-ray
//
//  Created by Valtteri on 23.12.2021.
//  Copyright © 2021-2022 Valtteri Koskivuori. All rights reserved.
//

#include <stddef.h>
#include "qoi.h"
#include "../../fileio.h"

#define QOI_IMPLEMENTATION
#include "../../../vendored/qoi.h"

void encode_qoi_from_array(const char *filename, const unsigned char *imgData, size_t width, size_t height) {
	int encoded_bytes = 0;
	const qoi_desc desc = (qoi_desc){
		.width = (int)width,
		.height = (int)height,
		.channels = 3,
		.colorspace = QOI_SRGB
	};
	const unsigned char *encoded_data = qoi_encode(imgData, &desc, &encoded_bytes);
	writeFile(encoded_data, encoded_bytes, filename);
}
