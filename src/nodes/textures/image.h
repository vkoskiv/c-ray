//
//  image.h
//  C-Ray
//
//  Created by Valtteri Koskivuori on 06/12/2020.
//  Copyright © 2020 Valtteri Koskivuori. All rights reserved.
//

#pragma once

//FIXME: These are opposite states, which is kinda confusing.
#define SRGB_TRANSFORM 0x01
#define NO_BILINEAR    0x02

const struct colorNode *newImageTexture(const struct node_storage *s, const struct texture *texture, uint8_t options);
