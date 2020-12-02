//
//  texturenode.h
//  C-Ray
//
//  Created by Valtteri Koskivuori on 30/11/2020.
//  Copyright © 2020 Valtteri Koskivuori. All rights reserved.
//

#pragma once

//TODO: Break this up into a color node, then expanding on that.

enum textureType {
	Diffuse,
	Normal,
	Specular
};

struct textureNode {
	struct color (*eval)(const struct textureNode *node, const struct hitRecord *record);
};

struct textureNode *newImageTexture(const struct texture *texture, uint8_t options);

struct textureNode *newConstantTexture(struct color color);

struct textureNode *newColorCheckerBoardTexture(const struct textureNode *colorA, const struct textureNode *colorB, float size);

struct textureNode *newCheckerBoardTexture(float size);
