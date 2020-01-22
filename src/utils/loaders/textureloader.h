//
//  textureloader.h
//  C-ray
//
//  Created by Valtteri Koskivuori on 02/04/2019.
//  Copyright © 2015-2020 Valtteri Koskivuori. All rights reserved.
//

#pragma once

//C-ray texture parser

/// Load a generic texture. Currently supports: JPEG, PNG, BMP, TGA, HDR, PIC, PNM
/// HDR files result in float type textures
/// @param filePath Path to image file on disk
struct texture *loadTexture(char *filePath);
