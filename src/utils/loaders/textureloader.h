//
//  textureloader.h
//  c-ray
//
//  Created by Valtteri Koskivuori on 02/04/2019.
//  Copyright © 2019-2023 Valtteri Koskivuori. All rights reserved.
//

#pragma once

#include "../fileio.h"

// Currently supports: JPEG, PNG, BMP, TGA, PIC, PNM, QOI, HDRI
struct texture *load_texture(const char *path, const file_data data);
