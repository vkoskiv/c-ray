//
//  textureloader.h
//  C-ray
//
//  Created by Valtteri Koskivuori on 02/04/2019.
//  Copyright © 2019-2020 Valtteri Koskivuori. All rights reserved.
//

#pragma once

struct block;

/// Load a generic texture. Currently supports: JPEG, PNG, BMP, TGA, PIC, PNM
/// @param filePath Path to image file on disk
/// @param pool Optional, memory pool to store image data
struct texture *loadTexture(char *filePath, struct block **pool);

struct texture *loadTextureFromBuffer(const unsigned char *buffer, const unsigned int buflen, struct block **pool);
