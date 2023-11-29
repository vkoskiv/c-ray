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

const struct bsdfNode *build_bsdf_node(struct cr_scene *s_ext, const struct cr_shader_node *desc) {
	if (!s_ext) return NULL;
	struct world *scene = (struct world *)s_ext;
	struct node_storage s = scene->storage;
	if (!desc) return warningBsdf(&s);
	switch (desc->type) {
		case cr_bsdf_diffuse:
			return newDiffuse(&s, build_color_node(s_ext, desc->arg.diffuse.color));
		case cr_bsdf_metal:
			return newMetal(&s, build_color_node(s_ext, desc->arg.metal.color), build_value_node(s_ext, desc->arg.metal.roughness));
		case cr_bsdf_glass:
			return newGlass(&s,
				build_color_node(s_ext, desc->arg.glass.color),
				build_value_node(s_ext, desc->arg.glass.roughness),
				build_value_node(s_ext, desc->arg.glass.IOR));
		case cr_bsdf_plastic:
			return newPlastic(&s,
				build_color_node(s_ext, desc->arg.plastic.color),
				build_value_node(s_ext, desc->arg.plastic.roughness),
				build_value_node(s_ext, desc->arg.plastic.IOR));
		case cr_bsdf_mix:
			return newMix(&s,
				build_bsdf_node(s_ext, desc->arg.mix.A),
				build_bsdf_node(s_ext, desc->arg.mix.B),
				build_value_node(s_ext, desc->arg.mix.factor));
		case cr_bsdf_add:
			return newAdd(&s, build_bsdf_node(s_ext, desc->arg.add.A), build_bsdf_node(s_ext, desc->arg.add.B));
		case cr_bsdf_transparent:
			return newTransparent(&s, build_color_node(s_ext, desc->arg.transparent.color));
		case cr_bsdf_emissive:
			return newEmission(&s, build_color_node(s_ext, desc->arg.emissive.color), build_value_node(s_ext, desc->arg.emissive.strength));
		case cr_bsdf_translucent:
			return newTranslucent(&s, build_color_node(s_ext, desc->arg.translucent.color));
		case cr_bsdf_background: {
			return newBackground(&s,
				build_color_node(s_ext, desc->arg.background.color),
				build_value_node(s_ext, desc->arg.background.strength),
				build_vector_node(s_ext, desc->arg.background.pose));
		}
		default:
			return warningBsdf(&s);
	};
}
