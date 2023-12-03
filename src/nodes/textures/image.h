//
//  image.h
//  c-ray
//
//  Created by Valtteri Koskivuori on 06/12/2020.
//  Copyright © 2020-2023 Valtteri Koskivuori. All rights reserved.
//

#pragma once

//FIXME: These are opposite states, which is kinda confusing.
#define SRGB_TRANSFORM 0x01
#define NO_BILINEAR    0x02

struct node_storage;
struct texture;

const struct colorNode *newImageTexture(const struct node_storage *s, const struct texture *texture, uint8_t options);
