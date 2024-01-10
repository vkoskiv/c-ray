//
//  colornode.h
//  c-ray
//
//  Created by Valtteri Koskivuori on 30/11/2020.
//  Copyright © 2020-2023 Valtteri Koskivuori. All rights reserved.
//

#pragma once

#include "../renderer/samplers/sampler.h"
#include "../datatypes/hitrecord.h"
#include <c-ray/c-ray.h>
#include "nodebase.h"

enum textureType {
	Diffuse,
	Normal,
	Specular
};

struct colorNode {
	struct nodeBase base;
	struct color (*eval)(const struct colorNode *node, sampler *sampler, const struct hitRecord *record);
};

#include "textures/checker.h"
#include "textures/constant.h"
#include "textures/image.h"
#include "textures/gradient.h"
#include "textures/colormix.h"
#include "converter/grayscale.h"
#include "converter/blackbody.h"
#include "converter/split.h"
#include "converter/combinergb.h"
#include "converter/combinehsl.h"
#include "converter/combinehsv.h"
#include "textures/hsv_transform.h"

// const struct colorNode *unknownTextureNode(const struct node_storage *s);

const struct colorNode *build_color_node(struct cr_scene *s_ext, const struct cr_color_node *desc);
