//
//  material.c
//  C-ray
//
//  Created by Valtteri Koskivuori on 20/05/2017.
//  Copyright © 2017-2022 Valtteri Koskivuori. All rights reserved.
//

#include "../includes.h"
#include "../utils/mempool.h"
#include "material.h"
#include "../utils/string.h"
#include <c-ray/c-ray.h>

#include "../renderer/pathtrace.h"
#include "image/texture.h"
#include "../datatypes/scene.h"

struct material_buffer *material_buf_ref(struct material_buffer *buf) {
	if (buf) {
		buf->refs++;
		return buf;
	}
	struct material_buffer *new = calloc(1, sizeof(*new));
	new->refs = 1;
	return new;
}

void material_buf_unref(struct material_buffer *buf) {
	if (!buf) return;
	if (--buf->refs) return;
	for (size_t i = 0; i < buf->materials.count; ++i) {
		destroyMaterial(&buf->materials.items[i]);
	}
	material_arr_free(&buf->materials);
	free(buf);
}

//To showcase missing .MTL file, for example
struct material warningMaterial() {
	struct material newMat = { 0 };
	newMat.type = lambertian;
	newMat.diffuse = (struct color){1.0f, 0.0f, 0.5f, 1.0f};
	return newMat;
}

// FIXME: Delete these and use ones in node.c instead
static struct bsdf_node_desc *alloc(struct bsdf_node_desc d) {
	struct bsdf_node_desc *desc = calloc(1, sizeof(*desc));
	memcpy(desc, &d, sizeof(*desc));
	return desc;
}

static struct value_node_desc *val_alloc(struct value_node_desc d) {
	struct value_node_desc *desc = calloc(1, sizeof(*desc));
	memcpy(desc, &d, sizeof(*desc));
	return desc;
}

static struct color_node_desc *col_alloc(struct color_node_desc d) {
	struct color_node_desc *desc = calloc(1, sizeof(*desc));
	memcpy(desc, &d, sizeof(*desc));
	return desc;
}
//FIXME: Temporary hack to patch alpha directly to old materials using the alpha node.
struct bsdf_node_desc *append_alpha(struct bsdf_node_desc *base, struct color_node_desc *color) {
	//FIXME: MSVC in release build mode crashes if we apply alpha on here, need to find out why. Just disable it for now though
#ifdef WINDOWS
	(void)color;
	return base;
#else
	return alloc((struct bsdf_node_desc){
		.type = cr_bsdf_mix,
		.arg.mix = {
			.A = alloc((struct bsdf_node_desc){
				.type = cr_bsdf_transparent,
				.arg.transparent.color = col_alloc((struct color_node_desc){
					.type = cr_cn_constant,
					.arg.constant = { g_white_color.red, g_white_color.green, g_white_color.blue, g_white_color.alpha }
				})
			}),
			.B = base,
			.factor = val_alloc((struct value_node_desc){
				.type = cr_vn_alpha,
				.arg.alpha.color = color
			})
		}
	});
#endif
}

struct color_node_desc *get_color(const struct material *mat) {
	if (mat->texture_path) {
		return col_alloc((struct color_node_desc){
			.type = cr_cn_image,
			.arg.image.options = SRGB_TRANSFORM,
			.arg.image.full_path = stringCopy(mat->texture_path)
		});
	} else {
		return col_alloc((struct color_node_desc){
			.type = cr_cn_constant,
			.arg.constant = { mat->diffuse.red, mat->diffuse.green, mat->diffuse.blue, mat->diffuse.alpha }
		});
	}
}

struct value_node_desc *get_rough(const struct material *mat) {
	if (mat->specular_path) {
		return val_alloc((struct value_node_desc){
			.type = cr_vn_grayscale,
			.arg.grayscale.color = col_alloc((struct color_node_desc){
				.type = cr_cn_image,
				.arg.image.options = NO_BILINEAR,
				.arg.image.full_path = stringCopy(mat->specular_path)
			})
		});
	} else {
		return val_alloc((struct value_node_desc){
			.type = cr_vn_constant,
			.arg.constant = (double)mat->roughness
		});
	}
}

struct bsdf_node_desc *try_to_guess_bsdf(const struct material *mat) {
	// FIXME: change material struct to only have path, and set up texture load here instead (nice!)

	logr(debug, "name: %s, illum: %i\n", mat->name, mat->illum);
	struct bsdf_node_desc *chosen_desc = NULL;
	
	// First, attempt to deduce type based on mtl properties
	switch (mat->illum) {
		case 5:
			chosen_desc = append_alpha(alloc((struct bsdf_node_desc){
				.type = cr_bsdf_metal,
				.arg.metal = {
					.color = get_color(mat),
					.roughness = get_rough(mat)
				}
			}), get_color(mat));
			break;
		case 7:
			chosen_desc = alloc((struct bsdf_node_desc){
				.type = cr_bsdf_glass,
				.arg.glass = {
					.color = col_alloc((struct color_node_desc){
						.type = cr_cn_constant,
						.arg.constant = { mat->specular.red, mat->specular.green, mat->specular.blue, mat->specular.alpha }
					}),
					.roughness = get_rough(mat),
					.IOR = val_alloc((struct value_node_desc){
						.type = cr_vn_constant,
						.arg.constant = (double)mat->IOR
					})
				}
			});
			break;
		default:
			break;
	}

	if (mat->emission.red > 0.0f ||
		mat->emission.blue > 0.0f ||
		mat->emission.green > 0.0f) {
		chosen_desc = alloc((struct bsdf_node_desc){
			.type = cr_bsdf_emissive,
			.arg.emissive = {
				.color = col_alloc((struct color_node_desc){
					.type = cr_cn_constant,
					.arg.constant = { mat->emission.red, mat->emission.green, mat->emission.blue, mat->emission.alpha }
				}),
				.strength = val_alloc((struct value_node_desc){
					.type = cr_vn_constant,
					.arg.constant = 1.0
				})
			}
		});
	}
	
	if (chosen_desc) goto skip;
	
	// Otherwise, fall back to our preassigned selection
	switch (mat->type) {
		case lambertian:
			chosen_desc = alloc((struct bsdf_node_desc){
				.type = cr_bsdf_diffuse,
				.arg.diffuse.color = get_color(mat)
			});
			break;
		case glass:
			chosen_desc = alloc((struct bsdf_node_desc){
				.type = cr_bsdf_glass,
				.arg.glass = {
					.color = get_color(mat),
					.roughness = get_rough(mat),
					.IOR = val_alloc((struct value_node_desc){
						.type = cr_vn_constant,
						.arg.constant = (double)mat->IOR
					})
				}
			});
			break;
		case metal:
			chosen_desc = alloc((struct bsdf_node_desc){
				.type = cr_bsdf_metal,
				.arg.metal = {
					.color = get_color(mat),
					.roughness = get_rough(mat),
				}
			});
			break;
		case plastic:
			chosen_desc = alloc((struct bsdf_node_desc){
				.type = cr_bsdf_plastic,
				.arg.plastic = {
					.color = get_color(mat),
					.roughness = get_rough(mat),
					.IOR = val_alloc((struct value_node_desc){
						.type = cr_vn_constant,
						.arg.constant = (double)mat->IOR
					})
				}
			});
			break;
		case emission:
			chosen_desc = alloc((struct bsdf_node_desc){
				.type = cr_bsdf_diffuse,
				.arg.diffuse.color = get_color(mat)
			});
			break;
		default:
			logr(warning, "Unknown bsdf type specified for \"%s\", setting to an obnoxious preset.\n", mat->name);
			chosen_desc = NULL;
			break;
	}

	skip:

	chosen_desc = append_alpha(chosen_desc, get_color(mat));

	return chosen_desc;
}

void destroyMaterial(struct material *mat) {
	if (mat) {
		free(mat->name);
		if (mat->texture_path) free(mat->texture_path);
		if (mat->normal_path) free(mat->normal_path);
		if (mat->specular_path) free(mat->specular_path);
	}
}
