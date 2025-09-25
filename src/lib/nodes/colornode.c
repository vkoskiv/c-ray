//
//  texturenode.c
//  c-ray
//
//  Created by Valtteri Koskivuori on 30/11/2020.
//  Copyright © 2020-2025 Valtteri Koskivuori. All rights reserved.
//

#include <v.h>
#include <renderer/samplers/sampler.h>
#include <renderer/renderer.h>
#include <common/color.h>
#include <common/vector.h>
#include <common/loaders/textureloader.h>
#include <common/cr_string.h>
#include <datatypes/poly.h>
#include <datatypes/scene.h>
#include "bsdfnode.h"
#include <common/platform/thread_pool.h>

#include "colornode.h"

// const struct colorNode *unknownTextureNode(const struct node_storage *s) {
// 	return newConstantTexture(s, g_black_color);
// }

struct decode_task_arg {
	char *path;
	struct texture *out;
	uint8_t options;
};

void tex_decode_task(void *arg) {
	struct decode_task_arg *dt = (struct decode_task_arg *)arg;
	v_timer timer = { 0 };
	v_timer_start(&timer);
	file_data data = file_load(dt->path);
	load_texture(dt->path, data, dt->out);
	long ms_decode = v_timer_get_ms(timer);
	//Since the texture is probably srgb, transform it back to linear colorspace for rendering
	if (dt->options & SRGB_TRANSFORM) tex_from_srgb(dt->out);
	long ms_total = v_timer_get_ms(timer);
	file_free(&data);
	char buf[3][64];
	logr(debug, "Async decode of '%s' took %s (%s dec, %s sRGB)\n",
		dt->path,
		ms_to_readable(ms_total, buf[0]),
		ms_to_readable(ms_decode, buf[1]),
		ms_to_readable(ms_total - ms_decode, buf[2]));
	free(dt->path);
	free(dt);
}

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
			if (!stringStartsWith(scene->asset_path, desc->arg.image.full_path))
				full = stringConcat(scene->asset_path, desc->arg.image.full_path);
			char *path = full ? full : desc->arg.image.full_path;
			windowsFixPath(path);
			struct texture *tex = NULL;
			// Note: We also deduplicate texture loads here, which ideally shouldn't be necessary.
			for (size_t i = 0; i < scene->textures.count; ++i) {
				if (stringEquals(scene->textures.items[i].path, path)) {
					tex = scene->textures.items[i].t;
				}
			}
			if (!tex) {
				tex = tex_new(none, 0, 0, 0);
				texture_asset_arr_add(&scene->textures, (struct texture_asset){
					.path = stringCopy(path),
					.t = tex,
				});
				struct decode_task_arg *arg = calloc(1, sizeof(*arg));
				*arg = (struct decode_task_arg){
					.path = stringCopy(path),
					.out = tex,
					.options = desc->arg.image.options
				};
				thread_pool_enqueue(scene->bg_worker, tex_decode_task, arg);
			}
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
		case cr_cn_hsv:
			return newCombineHSV(&s,
				build_value_node(s_ext, desc->arg.hsv.H),
				build_value_node(s_ext, desc->arg.hsv.S),
				build_value_node(s_ext, desc->arg.hsv.V));
		case cr_cn_hsv_tform:
			return newHSVTransform(&s,
				build_color_node(s_ext, desc->arg.hsv_tform.tex),
				build_value_node(s_ext, desc->arg.hsv_tform.H),
				build_value_node(s_ext, desc->arg.hsv_tform.S),
				build_value_node(s_ext, desc->arg.hsv_tform.V),
				build_value_node(s_ext, desc->arg.hsv_tform.f));
		case cr_cn_vec_to_color:
			return newVecToColor(&s, build_vector_node(s_ext, desc->arg.vec_to_color.vec));
		case cr_cn_color_mix:
			return new_color_mix(&s,
				build_color_node(s_ext, desc->arg.color_mix.a),
				build_color_node(s_ext, desc->arg.color_mix.b),
				build_value_node(s_ext, desc->arg.color_mix.factor));
		case cr_cn_color_ramp:
			return new_color_ramp(&s,
				build_value_node(s_ext, desc->arg.color_ramp.factor),
				desc->arg.color_ramp.color_mode,
				desc->arg.color_ramp.interpolation,
				desc->arg.color_ramp.elements,
				desc->arg.color_ramp.element_count);
		default: // FIXME: default remove
			return NULL;
	};
	return NULL;
}

