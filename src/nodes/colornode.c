//
//  texturenode.c
//  C-Ray
//
//  Created by Valtteri Koskivuori on 30/11/2020.
//  Copyright © 2020-2022 Valtteri Koskivuori. All rights reserved.
//

#include "../datatypes/color.h"
#include "../renderer/samplers/sampler.h"
#include "../renderer/renderer.h"
#include "../datatypes/vector.h"
#include "../datatypes/material.h"
#include "../datatypes/poly.h"
#include "../utils/string.h"
#include "../datatypes/scene.h"
#include "../utils/loaders/textureloader.h"
#include "../utils/loaders/sceneloader.h"
#include "bsdfnode.h"

#include "colornode.h"

// const struct colorNode *unknownTextureNode(const struct node_storage *s) {
// 	return newConstantTexture(s, g_black_color);
// }

const struct colorNode *build_color_node(struct cr_renderer *r_ext, const struct color_node_desc *desc) {
	if (!r_ext || !desc) return NULL;
	struct renderer *r = (struct renderer *)r_ext;
	struct file_cache *cache = r->state.file_cache;
	struct node_storage s = r->scene->storage;

	switch (desc->type) {
		case cr_cn_constant:
			return newConstantTexture(&s,
				(struct color){
					desc->arg.constant.r,
					desc->arg.constant.g,
					desc->arg.constant.b,
					desc->arg.constant.a
				});
		case cr_cn_image:
			return newImageTexture(&s, load_texture(desc->arg.image.full_path, &r->scene->storage.node_pool, cache), desc->arg.image.options);
		case cr_cn_checkerboard:
			return newCheckerBoardTexture(&s,
				build_color_node(r_ext, desc->arg.checkerboard.a),
				build_color_node(r_ext, desc->arg.checkerboard.b),
				build_value_node(r_ext, desc->arg.checkerboard.scale));
		case cr_cn_blackbody:
			return newBlackbody(&s, build_value_node(r_ext, desc->arg.blackbody.degrees));
		case cr_cn_split:
			return newSplitValue(&s, build_value_node(r_ext, desc->arg.split.node));
		case cr_cn_rgb:
			return newCombineRGB(&s,
				build_value_node(r_ext, desc->arg.rgb.red),
				build_value_node(r_ext, desc->arg.rgb.green),
				build_value_node(r_ext, desc->arg.rgb.blue));
		case cr_cn_hsl:
			return newCombineHSL(&s,
				build_value_node(r_ext, desc->arg.hsl.H),
				build_value_node(r_ext, desc->arg.hsl.S),
				build_value_node(r_ext, desc->arg.hsl.L));
		case cr_cn_vec_to_color:
			return newVecToColor(&s, build_vector_node(r_ext, desc->arg.vec_to_color.vec));
		default:
			return NULL;
	};
}

