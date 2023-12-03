//
//  texturenode.c
//  c-ray
//
//  Created by Valtteri Koskivuori on 30/11/2020.
//  Copyright © 2020-2023 Valtteri Koskivuori. All rights reserved.
//

#include "../../common/color.h"
#include "../renderer/samplers/sampler.h"
#include "../renderer/renderer.h"
#include "../../common/vector.h"
#include "../datatypes/poly.h"
#include "../../common/string.h"
#include "../datatypes/scene.h"
#include "../../common/loaders/textureloader.h"
#include "bsdfnode.h"

#include "colornode.h"

// const struct colorNode *unknownTextureNode(const struct node_storage *s) {
// 	return newConstantTexture(s, g_black_color);
// }

const struct colorNode *build_color_node(struct cr_scene *s_ext, const struct cr_color_node *desc) {
	if (!s_ext || !desc) return NULL;
	struct world *scene = (struct world *)s_ext;
	struct node_storage s = scene->storage;

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
			// FIXME: Hack, figure out a consistent way to deal with relative paths everywhere
			char *full = NULL;
			if (!stringStartsWith(scene->asset_path, desc->arg.image.full_path)) {
				full = stringConcat(scene->asset_path, desc->arg.image.full_path);
				windowsFixPath(full);
			}
			const char *path = full ? full : desc->arg.image.full_path;
			file_data data = file_load(path);
			struct texture *tex = NULL;
			// Note: We also deduplicate texture loads here, which ideally shouldn't be necessary.
			for (size_t i = 0; i < scene->textures.count; ++i) {
				if (stringEquals(scene->textures.items[i].path, path)) {
					tex = scene->textures.items[i].t;
				}
			}
			if (!tex) {
				tex = load_texture(path, data);
				texture_asset_arr_add(&scene->textures, (struct texture_asset){
					.path = stringCopy(path),
					.t = tex
				});
			}
			file_free(&data);
			const struct colorNode *new = newImageTexture(&s, tex, desc->arg.image.options);
			if (full) free(full);
			return new;
		}
		case cr_cn_checkerboard:
			return newCheckerBoardTexture(&s,
				build_color_node(s_ext, desc->arg.checkerboard.a),
				build_color_node(s_ext, desc->arg.checkerboard.b),
				build_value_node(s_ext, desc->arg.checkerboard.scale));
		case cr_cn_blackbody:
			return newBlackbody(&s, build_value_node(s_ext, desc->arg.blackbody.degrees));
		case cr_cn_split:
			return newSplitValue(&s, build_value_node(s_ext, desc->arg.split.node));
		case cr_cn_rgb:
			return newCombineRGB(&s,
				build_value_node(s_ext, desc->arg.rgb.red),
				build_value_node(s_ext, desc->arg.rgb.green),
				build_value_node(s_ext, desc->arg.rgb.blue));
		case cr_cn_hsl:
			return newCombineHSL(&s,
				build_value_node(s_ext, desc->arg.hsl.H),
				build_value_node(s_ext, desc->arg.hsl.S),
				build_value_node(s_ext, desc->arg.hsl.L));
		case cr_cn_vec_to_color:
			return newVecToColor(&s, build_vector_node(s_ext, desc->arg.vec_to_color.vec));
		default:
			return NULL;
	};
}

