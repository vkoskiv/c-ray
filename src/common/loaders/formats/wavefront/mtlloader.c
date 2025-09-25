//
//  mtlloader.c
//  c-ray
//
//  Created by Valtteri Koskivuori on 02/04/2019.
//  Copyright © 2019-2025 Valtteri Koskivuori. All rights reserved.
//

// c-ray MTL file parser

#include "../../../../includes.h"
#include "mtlloader.h"

#include <v.h>
#include <logging.h>
#include <cr_string.h>
#include <textbuffer.h>
#include <fileio.h>
#include <cr_assert.h>
#include <loaders/meshloader.h>
#include <color.h>
#include <texture.h>

/*
 From: https://blenderartists.org/forum/showthread.php?71202-Material-IOR-Value-reference
 'Air': 1.000
 'Bubble': 1.100
 'Liquid methane': 1.150
 'Ice(H2O)': 1.310
 'Water': 1.333
 'Clear Plastic': 1.400
 'Glass': 1.440 - 1.900
 'Light glass': 1.450
 'Standart glass': 1.520
 'Heavy glass': 1.650
 'Obsidian': 1.480 - 1.510
 'Onyx': 1.486 - 1.658
 'Acrylic glass': 1.491
 'Benzene': 1.501
 'Crown glass': 1.510
 'Jasper': 1.540
 'Agate': 1.544 - 1.553
 'Amethist': 1.544 - 1.553
 'Salt': 1.544
 'Amber': 1.550
 'Quartz': 1.550
 'Sugar': 1.560
 'Emerald': 1.576 - 1.582
 'Flint glass': 1.613
 'Topaz': 1.620 - 1.627
 'Jade': 1.660 - 1.680
 'Saphire': 1.760
 'Ruby': 1.760 - 2.419
 'Crystal': 1.870
 'Diamond': 2.417 - 2.541
 */

enum bsdfType {
	bt_none = 0,
	bt_emission,
	bt_lambertian,
	bt_glass,
	bt_plastic,
	bt_metal,
	bt_translucent,
	bt_transparent
};

struct material {
	char *name;
	char *texture_path;
	char *normal_path;
	char *specular_path;
	struct color diffuse;
	struct color specular;
	struct color emission;
	int illum;
	float shinyness;
	float reflectivity;
	float roughness;
	float refractivity;
	float IOR;
	float transparency;
	float sharpness;
	float glossiness;
	
	enum bsdfType type; // FIXME: Temporary
};

typedef struct material material;
v_arr_def(material)

// FIXME: Delete these and use ones in node.c instead
static struct cr_shader_node *alloc(struct cr_shader_node d) {
	struct cr_shader_node *desc = calloc(1, sizeof(*desc));
	memcpy(desc, &d, sizeof(*desc));
	return desc;
}

static struct cr_value_node *val_alloc(struct cr_value_node d) {
	struct cr_value_node *desc = calloc(1, sizeof(*desc));
	memcpy(desc, &d, sizeof(*desc));
	return desc;
}

static struct cr_color_node *col_alloc(struct cr_color_node d) {
	struct cr_color_node *desc = calloc(1, sizeof(*desc));
	memcpy(desc, &d, sizeof(*desc));
	return desc;
}
//FIXME: Temporary hack to patch alpha directly to old materials using the alpha node.
static struct cr_shader_node *append_alpha(struct cr_shader_node *base, struct cr_color_node *color) {
	//FIXME: MSVC in release build mode crashes if we apply alpha on here, need to find out why. Just disable it for now though
#ifdef WINDOWS
	(void)color;
	return base;
#else
	return alloc((struct cr_shader_node){
		.type = cr_bsdf_mix,
		.arg.mix = {
			.A = alloc((struct cr_shader_node){
				.type = cr_bsdf_transparent,
				.arg.transparent.color = col_alloc((struct cr_color_node){
					.type = cr_cn_constant,
					.arg.constant = { g_white_color.red, g_white_color.green, g_white_color.blue, g_white_color.alpha }
				})
			}),
			.B = base,
			.factor = val_alloc((struct cr_value_node){
				.type = cr_vn_alpha,
				.arg.alpha.color = color
			})
		}
	});
#endif
}

static struct cr_color_node *get_color(const struct material *mat) {
	if (mat->texture_path) {
		return col_alloc((struct cr_color_node){
			.type = cr_cn_image,
			.arg.image.options = SRGB_TRANSFORM,
			.arg.image.full_path = stringCopy(mat->texture_path)
		});
	} else {
		return col_alloc((struct cr_color_node){
			.type = cr_cn_constant,
			.arg.constant = { mat->diffuse.red, mat->diffuse.green, mat->diffuse.blue, mat->diffuse.alpha }
		});
	}
}

static struct cr_value_node *get_rough(const struct material *mat) {
	if (mat->specular_path) {
		return val_alloc((struct cr_value_node){
			.type = cr_vn_grayscale,
			.arg.grayscale.color = col_alloc((struct cr_color_node){
				.type = cr_cn_image,
				.arg.image.options = NO_BILINEAR,
				.arg.image.full_path = stringCopy(mat->specular_path)
			})
		});
	} else {
		return val_alloc((struct cr_value_node){
			.type = cr_vn_constant,
			.arg.constant = (double)mat->roughness
		});
	}
}

struct cr_shader_node *try_to_guess_bsdf(const struct material *mat) {

	logr(debug, "name: %s, illum: %i\n", mat->name, mat->illum);
	struct cr_shader_node *chosen_desc = NULL;
	
	// First, attempt to deduce type based on mtl properties
	switch (mat->illum) {
		case 5:
			chosen_desc = alloc((struct cr_shader_node){
				.type = cr_bsdf_metal,
				.arg.metal = {
					.color = get_color(mat),
					.roughness = get_rough(mat)
				}
			});
			break;
		case 7:
			chosen_desc = alloc((struct cr_shader_node){
				.type = cr_bsdf_glass,
				.arg.glass = {
					.color = col_alloc((struct cr_color_node){
						.type = cr_cn_constant,
						.arg.constant = { mat->specular.red, mat->specular.green, mat->specular.blue, mat->specular.alpha }
					}),
					.roughness = get_rough(mat),
					.IOR = val_alloc((struct cr_value_node){
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
		chosen_desc = alloc((struct cr_shader_node){
			.type = cr_bsdf_emissive,
			.arg.emissive = {
				.color = col_alloc((struct cr_color_node){
					.type = cr_cn_constant,
					.arg.constant = { mat->emission.red, mat->emission.green, mat->emission.blue, mat->emission.alpha }
				}),
				.strength = val_alloc((struct cr_value_node){
					.type = cr_vn_constant,
					.arg.constant = 1.0
				})
			}
		});
	}
	
	if (chosen_desc) goto skip;

	chosen_desc = alloc((struct cr_shader_node){
		.type = cr_bsdf_diffuse,
		.arg.diffuse.color = get_color(mat)
	});
	
	skip:

	return append_alpha(chosen_desc, get_color(mat));
}

static struct color parse_color(lineBuffer *line) {
	ASSERT(line->amountOf.tokens == 4);
	return (struct color){ atof(nextToken(line)), atof(nextToken(line)), atof(nextToken(line)), 1.0f };
}

void material_free(struct material *mat) {
	if (mat) {
		if (mat->name) free(mat->name);
		if (mat->texture_path) free(mat->texture_path);
		if (mat->normal_path) free(mat->normal_path);
		if (mat->specular_path) free(mat->specular_path);
	}
}

struct mesh_material_arr parse_mtllib(const char *filePath) {
	file_data mtllib_text = file_load(filePath);
	if (!mtllib_text.count) return (struct mesh_material_arr){ 0 };
	logr(debug, "Loading MTL at %s\n", filePath);
	textBuffer *file = newTextBuffer((char *)mtllib_text.items);
	file_free(&mtllib_text);

	char *asset_path = get_file_path(filePath);
	
	struct material_arr materials = { .elem_free = material_free };

	struct material *current = NULL;
	
	char *head = firstLine(file);
	char buf[LINEBUFFER_MAXSIZE];
	lineBuffer line = { .buf = buf };
	while (head) {
		fillLineBuffer(&line, head, ' ');
		char *first = firstToken(&line);
		if (first[0] == '#' || head[0] == '\0') {
			head = nextLine(file);
			continue;
		} else if (stringEquals(first, "newmtl")) {
			size_t idx = material_arr_add(&materials, (struct material){ 0 });
			current = &materials.items[idx];
			current->name = stringCopy(peekNextToken(&line));
			if (!current->name)
				logr(warning, "newmtl has no name in %s:%zu\n", filePath, line.current.line);
		} else if (stringEquals(first, "Ka")) {
			// Ignore
		} else if (current && stringEquals(first, "Kd")) {
			current->diffuse = parse_color(&line);
		} else if (current && stringEquals(first, "Ks")) {
			current->specular = parse_color(&line);
		} else if (current && stringEquals(first, "Ke")) {
			current->emission = parse_color(&line);
		} else if (current && stringEquals(first, "illum")) {
			current->illum = atoi(nextToken(&line));
		} else if (current && stringEquals(first, "Ns")) {
			current->shinyness = atof(nextToken(&line));
		} else if (current && stringEquals(first, "d")) {
			current->transparency = atof(nextToken(&line));
		} else if (current && stringEquals(first, "r")) {
			current->reflectivity = atof(nextToken(&line));
		} else if (current && stringEquals(first, "sharpness")) {
			current->glossiness = atof(nextToken(&line));
		} else if (current && stringEquals(first, "Ni")) {
			current->IOR = atof(nextToken(&line));
		} else if (current && (stringEquals(first, "map_Kd") || stringEquals(first, "map_Ka"))) {
			current->texture_path = stringConcat(asset_path, peekNextToken(&line));
		} else if (current && (stringEquals(first, "norm") ||
		                       stringEquals(first, "bump") ||
		                       stringEquals(first, "map_bump"))) {
			current->normal_path = stringConcat(asset_path, peekNextToken(&line));
		} else if (current && stringEquals(first, "map_Ns")) {
			current->specular_path = stringConcat(asset_path, peekNextToken(&line));
		} else if (current) {
			logr(debug, "Unknown statement \"%s\" in %s:%zu\n", first, filePath, file->current.line);
		}
		head = nextLine(file);
	}

	if (asset_path) free(asset_path);
	
	destroyTextBuffer(file);
	logr(materials.count ? debug : warning, "Found %zu material%s\n", materials.count, PLURAL(materials.count));
	struct mesh_material_arr out = { 0 };
	for (size_t i = 0; i < materials.count; ++i) {
		mesh_material_arr_add(&out, (struct mesh_material){
			.mat = try_to_guess_bsdf(&materials.items[i]),
			.name = stringCopy(materials.items[i].name)
		});
	}
	material_arr_free(&materials);
	return out;
}
