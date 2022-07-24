//
//  bsdfnode.h
//  C-Ray
//
//  Created by Valtteri Koskivuori on 29/11/2020.
//  Copyright © 2020-2022 Valtteri Koskivuori. All rights reserved.
//

#pragma once

#include "../vendored/cJSON.h"
#include "../datatypes/lightray.h"
#include "../utils/mempool.h"
#include "valuenode.h"
#include "vectornode.h"
#include "colornode.h"
#include "../datatypes/hitrecord.h"
#include "nodebase.h"

struct bsdfSample {
	struct vector out;
	float pdf;
	struct color color;
};

//TODO: Expand and refactor to match a standard bsdf signature with eval, sample and pdf
struct bsdfNode {
	struct nodeBase base;
	struct bsdfSample (*sample)(const struct bsdfNode *bsdf, sampler *sampler, const struct hitRecord *record);
};

#include "shaders/diffuse.h"
#include "shaders/glass.h"
#include "shaders/metal.h"
#include "shaders/mix.h"
#include "shaders/plastic.h"
#include "shaders/transparent.h"
#include "shaders/add.h"
#include "shaders/background.h"
#include "shaders/emission.h"
#include "shaders/isotropic.h"
#include "shaders/translucent.h"

const struct bsdfNode *warningBsdf(const struct node_storage *s);
const struct bsdfNode *parseBsdfNode(const char *asset_path, struct file_cache *cache, struct node_storage *s, const cJSON *node);
