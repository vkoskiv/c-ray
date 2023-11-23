//
//  bsdfnode.c
//  C-Ray
//
//  Created by Valtteri Koskivuori on 29/11/2020.
//  Copyright © 2020-2022 Valtteri Koskivuori. All rights reserved.
//

#include "../datatypes/vector.h"
#include "../datatypes/color.h"
#include "../datatypes/material.h"
#include "../renderer/renderer.h"
#include "../datatypes/scene.h"
#include "../utils/string.h"
#include "bsdfnode.h"
#include <c-ray/c-ray.h>

struct bsdf_buffer *bsdf_buf_ref(struct bsdf_buffer *buf) {
	if (buf) {
		buf->refs++;
		return buf;
	}
	struct bsdf_buffer *new = calloc(1, sizeof(*new));
	new->refs = 1;
	return new;
}

void bsdf_buf_unref(struct bsdf_buffer *buf) {
	if (!buf) return;
	if (--buf->refs) return;
	bsdf_node_ptr_arr_free(&buf->bsdfs);
	free(buf);
}

const struct bsdfNode *build_bsdf_node(struct cr_renderer *r_ext, const struct cr_shader_node *desc) {
	if (!r_ext) return NULL;
	struct renderer *r = (struct renderer *)r_ext;
	struct node_storage s = r->scene->storage;
	if (!desc) return warningBsdf(&s);
	switch (desc->type) {
		case cr_bsdf_diffuse:
			return newDiffuse(&s, build_color_node(r_ext, desc->arg.diffuse.color));
		case cr_bsdf_metal:
			return newMetal(&s, build_color_node(r_ext, desc->arg.metal.color), build_value_node(r_ext, desc->arg.metal.roughness));
		case cr_bsdf_glass:
			return newGlass(&s,
				build_color_node(r_ext, desc->arg.glass.color),
				build_value_node(r_ext, desc->arg.glass.roughness),
				build_value_node(r_ext, desc->arg.glass.IOR));
		case cr_bsdf_plastic:
			return newPlastic(&s,
				build_color_node(r_ext, desc->arg.plastic.color),
				build_value_node(r_ext, desc->arg.plastic.roughness),
				build_value_node(r_ext, desc->arg.plastic.IOR));
		case cr_bsdf_mix:
			return newMix(&s,
				build_bsdf_node(r_ext, desc->arg.mix.A),
				build_bsdf_node(r_ext, desc->arg.mix.B),
				build_value_node(r_ext, desc->arg.mix.factor));
		case cr_bsdf_add:
			return newAdd(&s, build_bsdf_node(r_ext, desc->arg.add.A), build_bsdf_node(r_ext, desc->arg.add.B));
		case cr_bsdf_transparent:
			return newTransparent(&s, build_color_node(r_ext, desc->arg.transparent.color));
		case cr_bsdf_emissive:
			return newEmission(&s, build_color_node(r_ext, desc->arg.emissive.color), build_value_node(r_ext, desc->arg.emissive.strength));
		case cr_bsdf_translucent:
			return newTranslucent(&s, build_color_node(r_ext, desc->arg.translucent.color));
		case cr_bsdf_background: {
			// FIXME: Yuck
			// Also, this will shit the bed if that path isn't a valid one
			if (desc->arg.background.color->type == cr_cn_image && desc->arg.background.color->arg.image.full_path) {
				char *path = stringConcat(r->prefs.assetPath, desc->arg.background.color->arg.image.full_path);
				windowsFixPath(path);
				free(desc->arg.background.color->arg.image.full_path);
				desc->arg.background.color->arg.image.full_path = path;
			}
			return newBackground(&s,
				build_color_node(r_ext, desc->arg.background.color),
				build_value_node(r_ext, desc->arg.background.strength),
				build_vector_node(r_ext, desc->arg.background.pose));
		}
		default:
			return warningBsdf(&s);
	};
}
