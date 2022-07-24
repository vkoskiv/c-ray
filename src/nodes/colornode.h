//
//  colornode.h
//  C-Ray
//
//  Created by Valtteri Koskivuori on 30/11/2020.
//  Copyright © 2020-2022 Valtteri Koskivuori. All rights reserved.
//

#pragma once

#include "../vendored/cJSON.h"
#include "nodebase.h"

enum textureType {
	Diffuse,
	Normal,
	Specular
};

struct colorNode {
	struct nodeBase base;
	struct color (*eval)(const struct colorNode *node, const struct hitRecord *record);
};

#include "textures/checker.h"
#include "textures/constant.h"
#include "textures/image.h"
#include "textures/gradient.h"
#include "converter/grayscale.h"
#include "converter/blackbody.h"
#include "converter/split.h"
#include "converter/combinergb.h"
#include "converter/combinehsl.h"

const struct colorNode *unknownTextureNode(const struct node_storage *s);

struct file_cache;
// Not ideal, but for now we have to pass asset_path and file cache in here for texture loading
const struct colorNode *parseTextureNode(const char *asset_path, struct file_cache *cache, struct node_storage *s, const cJSON *node);
