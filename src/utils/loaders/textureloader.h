//
//  textureloader.h
//  C-ray
//
//  Created by Valtteri Koskivuori on 02/04/2019.
//  Copyright © 2019-2021 Valtteri Koskivuori. All rights reserved.
//

#pragma once

struct block;
struct file_cache;

/// Load a generic texture. Currently supports: JPEG, PNG, BMP, TGA, PIC, PNM, QOI
/// @param filePath Path to image file on disk
/// @param pool Optional, memory pool to store image data
struct texture *load_texture(char *filePath, struct block **pool, struct file_cache *cache);

// Only exposed for the built-in logo if SDL is enabled.
struct texture *load_texture_from_buffer(const unsigned char *buffer, const unsigned int buflen, struct block **pool);
