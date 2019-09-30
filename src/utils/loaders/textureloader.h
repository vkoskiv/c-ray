//
//  textureloader.h
//  C-ray
//
//  Created by Valtteri Koskivuori on 02/04/2019.
//  Copyright © 2015-2019 Valtteri Koskivuori. All rights reserved.
//

#pragma once

//C-ray texture parser

//Load a generic texture. Currently only PNG and Radiance HDR supported.
struct texture *loadTexture(char *filePath);
