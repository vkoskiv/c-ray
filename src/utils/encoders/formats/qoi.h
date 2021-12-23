//
//  qoi.h
//  C-ray
//
//  Created by Valtteri on 23.12.2021.
//  Copyright © 2021 Valtteri Koskivuori. All rights reserved.
//

#pragma once

void encode_qoi_from_array(const char *filename, unsigned char *imgData, size_t width, size_t height);
