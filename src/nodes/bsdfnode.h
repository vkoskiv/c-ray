//
//  bsdfnode.h
//  c-ray
//
//  Created by Valtteri Koskivuori on 29/11/2020.
//  Copyright © 2020-2023 Valtteri Koskivuori. All rights reserved.
//

#pragma once

#include "../datatypes/lightray.h"
#include "../datatypes/color.h"
#include "../utils/mempool.h"
#include "../utils/dyn_array.h"
#include "valuenode.h"
#include "vectornode.h"
#include "colornode.h"
#include "../datatypes/hitrecord.h"
#include "nodebase.h"

struct bsdfSample {
	struct vector out;
	float pdf;
	struct color weight;
	struct color emitted; // FIXME: Not really the right place for this
};

//TODO: Expand and refactor to match a standard bsdf signature with eval, sample and pdf
struct bsdfNode {
	struct nodeBase base;
	struct bsdfSample (*sample)(const struct bsdfNode *bsdf, sampler *sampler, const struct hitRecord *record);
};

typedef const struct bsdfNode * bsdf_node_ptr;
dyn_array_def(bsdf_node_ptr);

typedef struct cr_shader_node * cr_shader_node_ptr;
dyn_array_def(cr_shader_node_ptr);

struct bsdf_buffer {
	struct bsdf_node_ptr_arr bsdfs;
	struct cr_shader_node_ptr_arr descriptions;
};

void bsdf_buffer_free(struct bsdf_buffer *b);

typedef struct bsdf_buffer bsdf_buffer;
dyn_array_def(bsdf_buffer);

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

const struct bsdfNode *build_bsdf_node(struct cr_scene *s_ext, const struct cr_shader_node *desc);
