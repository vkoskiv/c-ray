//
//  texturenode.c
//  c-ray
//
//  Created by Valtteri Koskivuori on 30/11/2020.
//  Copyright © 2020-2023 Valtteri Koskivuori. All rights reserved.
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
#include "bsdfnode.h"

#include "colornode.h"

// const struct colorNode *unknownTextureNode(const struct node_storage *s) {
// 	return newConstantTexture(s, g_black_color);
// }

const struct colorNode *build_color_node(struct cr_renderer *r_ext, const struct cr_color_node *desc) {
	if (!r_ext || !desc) return NULL;
	struct renderer *r = (struct renderer *)r_ext;
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
		case cr_cn_image: {
			char *full = stringConcat(r->prefs.assetPath, desc->arg.image.full_path);
			windowsFixPath(full);
			file_data tex;
			if (cache_contains(r->state.file_cache, full)) {
				tex = cache_load(r->state.file_cache, full);
			} else {
				tex = file_load(full, NULL);
				cache_store(r->state.file_cache, full, tex.items, tex.count);
			}
			const struct colorNode *new = newImageTexture(&s, load_texture(full, tex, &r->scene->storage.node_pool), desc->arg.image.options);
			free(full);
			file_free(&tex);
			return new;
		}
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

