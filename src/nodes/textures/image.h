//
//  image.h
//  C-Ray
//
//  Created by Valtteri Koskivuori on 06/12/2020.
//  Copyright © 2020 Valtteri Koskivuori. All rights reserved.
//

#pragma once

#define SRGB_TRANSFORM 0x01
#define NO_BILINEAR    0x02

struct colorNode *newImageTexture(struct world *world, struct texture *texture, uint8_t options);
