//
//  node.c
//  c-ray
//
//  Created by Valtteri Koskivuori on 19/11/2023.
//  Copyright © 2023 Valtteri Koskivuori. All rights reserved.
//

#include <c-ray/c-ray.h>

#include "../renderer/renderer.h"
#include "../datatypes/scene.h"
#include "../utils/loaders/textureloader.h"
#include "../utils/string.h"
#include "../vendored/cJSON.h"
#include "../utils/logging.h"
#include "c-ray/node.h"

static enum cr_vec_to_value_component value_node_component(const cJSON *data) {
	if (!cJSON_IsString(data)) {
		logr(warning, "No component specified for vecToValue node, defaulting to scalar (F).\n");
		return F;
	}
	if (stringEquals(data->valuestring, "x")) return X;
	if (stringEquals(data->valuestring, "y")) return Y;
	if (stringEquals(data->valuestring, "z")) return Z;
	if (stringEquals(data->valuestring, "u")) return U;
	if (stringEquals(data->valuestring, "v")) return V;
	if (stringEquals(data->valuestring, "f")) return F;
	logr(warning, "Unknown component specified: %s\n", data->valuestring);
	return F;
}

static enum cr_math_op value_node_op(const cJSON *data) {
	if (!cJSON_IsString(data)) {
		logr(warning, "No math op given, defaulting to add.\n");
		return Add;
	}
	if (stringEquals(data->valuestring, "add")) return Add;
	if (stringEquals(data->valuestring, "subtract")) return Subtract;
	if (stringEquals(data->valuestring, "multiply")) return Multiply;
	if (stringEquals(data->valuestring, "divide")) return Divide;
	if (stringEquals(data->valuestring, "power")) return Power;
	if (stringEquals(data->valuestring, "log")) return Log;
	if (stringEquals(data->valuestring, "sqrt")) return SquareRoot;
	if (stringEquals(data->valuestring, "invsqrt")) return InvSquareRoot;
	if (stringEquals(data->valuestring, "abs")) return Absolute;
	if (stringEquals(data->valuestring, "min")) return Min;
	if (stringEquals(data->valuestring, "max")) return Max;
	if (stringEquals(data->valuestring, "lt")) return LessThan;
	if (stringEquals(data->valuestring, "gt")) return GreaterThan;
	if (stringEquals(data->valuestring, "sign")) return Sign;
	if (stringEquals(data->valuestring, "compare")) return Compare;
	if (stringEquals(data->valuestring, "round")) return Round;
	if (stringEquals(data->valuestring, "floor")) return Floor;
	if (stringEquals(data->valuestring, "ceil")) return Ceil;
	if (stringEquals(data->valuestring, "truncate")) return Truncate;
	if (stringEquals(data->valuestring, "fraction")) return Fraction;
	if (stringEquals(data->valuestring, "mod")) return Modulo;
	if (stringEquals(data->valuestring, "sin")) return Sine;
	if (stringEquals(data->valuestring, "cos")) return Cosine;
	if (stringEquals(data->valuestring, "tan")) return Tangent;
	if (stringEquals(data->valuestring, "toradians")) return ToRadians;
	if (stringEquals(data->valuestring, "todegrees")) return ToDegrees;
	logr(warning, "Unknown math op %s given, defaulting to add\n", data->valuestring);
	return Add;
}

static struct cr_value_node *vn_alloc(struct cr_value_node d) {
	struct cr_value_node *desc = calloc(1, sizeof(*desc));
	memcpy(desc, &d, sizeof(*desc));
	return desc;
}

struct cr_value_node *build_value_node_desc(const struct cJSON *node) {
	if (!node) return NULL;
	if (cJSON_IsNumber(node)) {
		return vn_alloc((struct cr_value_node){
			.type = cr_vn_constant,
			.arg.constant = node->valuedouble
		});
	}

	const cJSON *type = cJSON_GetObjectItem(node, "type");
	if (!cJSON_IsString(type)) {
		return vn_alloc((struct cr_value_node){
			.type = cr_vn_grayscale,
			.arg.grayscale.color = build_color_node_desc(node)
		});
	}

	if (stringEquals(type->valuestring, "fresnel")) {
		return vn_alloc((struct cr_value_node){
			.type = cr_vn_fresnel,
			.arg.fresnel = {
				.IOR = build_value_node_desc(cJSON_GetObjectItem(node, "IOR")),
				.normal = build_vector_node_desc(cJSON_GetObjectItem(node, "normal"))
			}
		});
	}
	if (stringEquals(type->valuestring, "map_range")) {
		return vn_alloc((struct cr_value_node){
			.type = cr_vn_map_range,
			.arg.map_range = {
				.input_value = build_value_node_desc(cJSON_GetObjectItem(node, "input")),
				.from_min = build_value_node_desc(cJSON_GetObjectItem(node, "from_min")),
				.from_max = build_value_node_desc(cJSON_GetObjectItem(node, "from_max")),
				.to_min = build_value_node_desc(cJSON_GetObjectItem(node, "to_min")),
				.to_max = build_value_node_desc(cJSON_GetObjectItem(node, "to_max")),
			}
		});
	}
	if (stringEquals(type->valuestring, "raylength")) {
		return vn_alloc((struct cr_value_node){
			.type = cr_vn_raylength,
		});
	}
	if (stringEquals(type->valuestring, "alpha")) {
		return vn_alloc((struct cr_value_node){
			.type = cr_vn_alpha,
			.arg.alpha.color = build_color_node_desc(cJSON_GetObjectItem(node, "color"))
		});
	}
	if (stringEquals(type->valuestring, "vec_to_value")) {
		return vn_alloc((struct cr_value_node){
			.type = cr_vn_vec_to_value,
			.arg.vec_to_value = {
				.vec = build_vector_node_desc(cJSON_GetObjectItem(node, "vector")),
				.comp = value_node_component(cJSON_GetObjectItem(node, "component"))
			}
		});
	}
	if (stringEquals(type->valuestring, "math")) {
		return vn_alloc((struct cr_value_node){
			.type = cr_vn_math,
			.arg.math = {
				.A = build_value_node_desc(cJSON_GetObjectItem(node, "a")),
				.B = build_value_node_desc(cJSON_GetObjectItem(node, "b")),
				.op = value_node_op(cJSON_GetObjectItem(node, "op"))
			}
		});
	}
	return vn_alloc((struct cr_value_node){
		.type = cr_vn_grayscale,
		.arg.grayscale.color = build_color_node_desc(cJSON_GetObjectItem(node, "color"))
	});
}

void cr_node_value_desc_del(struct cr_value_node *d) {
	if (!d) return;
	switch (d->type) {
		case cr_vn_unknown:
			break;
		case cr_vn_constant:
			break;
		case cr_vn_fresnel:
			cr_node_value_desc_del(d->arg.fresnel.IOR);
			cr_node_vector_desc_del(d->arg.fresnel.normal);
			break;
		case cr_vn_map_range:
			cr_node_value_desc_del(d->arg.map_range.input_value);
			cr_node_value_desc_del(d->arg.map_range.from_min);
			cr_node_value_desc_del(d->arg.map_range.from_max);
			cr_node_value_desc_del(d->arg.map_range.to_min);
			cr_node_value_desc_del(d->arg.map_range.to_max);
			break;
		case cr_vn_raylength:
			break;
		case cr_vn_alpha:
			cr_node_color_desc_del(d->arg.alpha.color);
			break;
		case cr_vn_vec_to_value:
			cr_node_vector_desc_del(d->arg.vec_to_value.vec);
			break;
		case cr_vn_math:
			cr_node_value_desc_del(d->arg.math.A);
			cr_node_value_desc_del(d->arg.math.B);
			break;
		case cr_vn_grayscale:
			cr_node_color_desc_del(d->arg.grayscale.color);
	}
	free(d);
}

static struct cr_color_node *cn_alloc(struct cr_color_node d) {
	struct cr_color_node *desc = calloc(1, sizeof(*desc));
	memcpy(desc, &d, sizeof(*desc));
	return desc;
}

struct cr_color_node *build_color_node_desc(const struct cJSON *desc) {
	if (!desc) return NULL;

	if (cJSON_IsArray(desc)) {
		struct color c = color_parse(desc);
		return cn_alloc((struct cr_color_node){
			.type = cr_cn_constant,
			.arg.constant = { c.red, c.green, c.blue, c.alpha }
		});
	}

	// Handle options first
	uint8_t options = 0;
	options |= SRGB_TRANSFORM; // Enabled by default.

	if (cJSON_IsString(desc)) {
		// No options provided, go with defaults.
		return cn_alloc((struct cr_color_node){
			.type = cr_cn_image,
			.arg.image.full_path = stringCopy(desc->valuestring),
			.arg.image.options = options
		});
	}

	// Should be an object, then.
	if (!cJSON_IsObject(desc)) {
		logr(warning, "Invalid texture node given: \"%s\"\n", cJSON_PrintUnformatted(desc));
		return NULL;
	}

	// Do we want to do an srgb transform?
	const cJSON *srgbTransform = cJSON_GetObjectItem(desc, "transform");
	if (srgbTransform) {
		if (!cJSON_IsTrue(srgbTransform)) {
			options &= ~SRGB_TRANSFORM;
		}
	}

	// Do we want bilinear interpolation enabled?
	const cJSON *lerp = cJSON_GetObjectItem(desc, "lerp");
	if (!cJSON_IsTrue(lerp)) {
		options |= NO_BILINEAR;
	}

	const cJSON *path = cJSON_GetObjectItem(desc, "path");
	if (cJSON_IsString(path)) {
		return cn_alloc((struct cr_color_node){
			.type = cr_cn_image,
			.arg.image.full_path = stringCopy(path->valuestring),
			.arg.image.options = options
		});
	}

	//FIXME: No good way to know if it's a color, so just check if it's got "r" in there.
	if (cJSON_HasObjectItem(desc, "r")) {
		// This is actually still a color object.
		struct color c = color_parse(desc);
		return cn_alloc((struct cr_color_node){
			.type = cr_cn_constant,
			.arg.constant = { c.red, c.green, c.blue, c.alpha }
		});
	}
	const cJSON *type = cJSON_GetObjectItem(desc, "type");
	if (cJSON_IsString(type)) {
		// Oo, what's this?
		if (stringEquals(type->valuestring, "checkerboard")) {
			return cn_alloc((struct cr_color_node){
				.type = cr_cn_checkerboard,
				.arg.checkerboard = {
					.a = build_color_node_desc(cJSON_GetObjectItem(desc, "color1")),
					.b = build_color_node_desc(cJSON_GetObjectItem(desc, "color2")),
					.scale = build_value_node_desc(cJSON_GetObjectItem(desc, "scale"))
				}
			});
		}
		if (stringEquals(type->valuestring, "blackbody")) {
			// FIXME: valuenode
			return cn_alloc((struct cr_color_node){
				.type = cr_cn_blackbody,
				.arg.blackbody.degrees = build_value_node_desc(cJSON_GetObjectItem(desc, "degrees"))
			});
		}
		if (stringEquals(type->valuestring, "split")) {
			return cn_alloc((struct cr_color_node){
				.type = cr_cn_split,
				.arg.split.node = build_value_node_desc(cJSON_GetObjectItem(desc, "constant"))
			});
		}
		if (stringEquals(type->valuestring, "rgb")) {
			return cn_alloc((struct cr_color_node){
				.type = cr_cn_rgb,
				.arg.rgb = {
					.red = build_value_node_desc(cJSON_GetObjectItem(desc, "r")),
					.green = build_value_node_desc(cJSON_GetObjectItem(desc, "g")),
					.blue = build_value_node_desc(cJSON_GetObjectItem(desc, "b")),
				}
			});
		}
		if (stringEquals(type->valuestring, "hsl")) {
			return cn_alloc((struct cr_color_node){
				.type = cr_cn_hsl,
				.arg.hsl = {
					.H = build_value_node_desc(cJSON_GetObjectItem(desc, "h")),
					.S = build_value_node_desc(cJSON_GetObjectItem(desc, "s")),
					.L = build_value_node_desc(cJSON_GetObjectItem(desc, "l")),
				}
			});
		}
		if (stringEquals(type->valuestring, "to_color")) {
			return cn_alloc((struct cr_color_node){
				.type = cr_cn_vec_to_color,
				.arg.vec_to_color.vec = build_vector_node_desc(cJSON_GetObjectItem(desc, "vector"))
			});
		}
	}

	logr(warning, "Failed to parse textureNode. Here's a dump:\n");
	logr(warning, "\n%s\n", cJSON_Print(desc));
	logr(warning, "Setting to an obnoxious pink material.\n");
	return NULL;

}

void cr_node_color_desc_del(struct cr_color_node *d) {
	if (!d) return;
	switch (d->type) {
		case cr_cn_unknown:
		case cr_cn_constant:
			break;
		case cr_cn_image:
			if (d->arg.image.full_path) {
				free(d->arg.image.full_path);
				d->arg.image.full_path = NULL;
			}
			break;
		case cr_cn_checkerboard:
			cr_node_color_desc_del(d->arg.checkerboard.a);
			cr_node_color_desc_del(d->arg.checkerboard.b);
			cr_node_value_desc_del(d->arg.checkerboard.scale);
			break;
		case cr_cn_blackbody:
			cr_node_value_desc_del(d->arg.blackbody.degrees);
			break;
		case cr_cn_split:
			cr_node_value_desc_del(d->arg.split.node);
			break;
		case cr_cn_rgb:
			cr_node_value_desc_del(d->arg.rgb.red);
			cr_node_value_desc_del(d->arg.rgb.green);
			cr_node_value_desc_del(d->arg.rgb.blue);
			break;
		case cr_cn_hsl:
			cr_node_value_desc_del(d->arg.hsl.H);
			cr_node_value_desc_del(d->arg.hsl.S);
			cr_node_value_desc_del(d->arg.hsl.L);
			break;
		case cr_cn_vec_to_color:
			cr_node_vector_desc_del(d->arg.vec_to_color.vec);
			break;
	}
	free(d);
}

static struct cr_vector_node *vecn_alloc(struct cr_vector_node d) {
	struct cr_vector_node *desc = calloc(1, sizeof(*desc));
	memcpy(desc, &d, sizeof(*desc));
	return desc;
}

static enum cr_vec_op parseVectorOp(const cJSON *data) {
	if (!cJSON_IsString(data)) {
		logr(warning, "No vector op given, defaulting to add.\n");
		return VecAdd;
	}
	if (stringEquals(data->valuestring, "add")) return VecAdd;
	if (stringEquals(data->valuestring, "subtract")) return VecSubtract;
	if (stringEquals(data->valuestring, "multiply")) return VecMultiply;
	if (stringEquals(data->valuestring, "divide")) return VecDivide;
	if (stringEquals(data->valuestring, "cross")) return VecCross;
	if (stringEquals(data->valuestring, "reflect")) return VecReflect;
	if (stringEquals(data->valuestring, "refract")) return VecRefract;
	if (stringEquals(data->valuestring, "dot")) return VecDot;
	if (stringEquals(data->valuestring, "distance")) return VecDistance;
	if (stringEquals(data->valuestring, "length")) return VecLength;
	if (stringEquals(data->valuestring, "scale")) return VecScale;
	if (stringEquals(data->valuestring, "normalize")) return VecNormalize;
	if (stringEquals(data->valuestring, "wrap")) return VecWrap;
	if (stringEquals(data->valuestring, "floor")) return VecFloor;
	if (stringEquals(data->valuestring, "ceil")) return VecCeil;
	if (stringEquals(data->valuestring, "mod")) return VecModulo;
	if (stringEquals(data->valuestring, "abs")) return VecAbs;
	if (stringEquals(data->valuestring, "min")) return VecMin;
	if (stringEquals(data->valuestring, "max")) return VecMax;
	if (stringEquals(data->valuestring, "sin")) return VecSin;
	if (stringEquals(data->valuestring, "cos")) return VecCos;
	if (stringEquals(data->valuestring, "tan")) return VecTan;
	logr(warning, "Unknown vector op %s given, defaulting to add\n", data->valuestring);
	return VecAdd;
}

struct vector parseVector(const struct cJSON *data) {
	const float x = cJSON_IsNumber(cJSON_GetArrayItem(data, 0)) ? (float)cJSON_GetArrayItem(data, 0)->valuedouble : 0.0f;
	const float y = cJSON_IsNumber(cJSON_GetArrayItem(data, 1)) ? (float)cJSON_GetArrayItem(data, 1)->valuedouble : 0.0f;
	const float z = cJSON_IsNumber(cJSON_GetArrayItem(data, 2)) ? (float)cJSON_GetArrayItem(data, 2)->valuedouble : 0.0f;
	return (struct vector){ x, y, z };
}

struct cr_vector_node *build_vector_node_desc(const struct cJSON *node) {
	if (!node) return NULL;
	if (cJSON_IsArray(node)) {
		struct vector v = parseVector(node);
		return vecn_alloc((struct cr_vector_node){
			.type = cr_vec_constant,
			.arg.constant = { v.x, v.y, v.z }
		});
	}
	const cJSON *type = cJSON_GetObjectItem(node, "type");
	if (!cJSON_IsString(type)) {
		logr(warning, "No type provided for vectorNode: %s\n", cJSON_PrintUnformatted(node));
		return NULL;
	}

	if (stringEquals(type->valuestring, "normal")) {
		return vecn_alloc((struct cr_vector_node){
			.type = cr_vec_normal
		});
	}
	if (stringEquals(type->valuestring, "uv")) {
		return vecn_alloc((struct cr_vector_node){
			.type = cr_vec_uv
		});
	}

	if (stringEquals(type->valuestring, "vecmath")) {
		return vecn_alloc((struct cr_vector_node){
			.type = cr_vec_vecmath,
			.arg.vecmath = {
				.A = build_vector_node_desc(cJSON_GetObjectItem(node, "a")),
				.B = build_vector_node_desc(cJSON_GetObjectItem(node, "b")),
				.C = build_vector_node_desc(cJSON_GetObjectItem(node, "c")),
				.f = build_value_node_desc(cJSON_GetObjectItem(node, "f")),
				.op = parseVectorOp(cJSON_GetObjectItem(node, "op"))
			}
		});
	}
	if (stringEquals(type->valuestring, "mix")) {
		return vecn_alloc((struct cr_vector_node){
			.type = cr_vec_mix,
			.arg.vec_mix = {
				.A = build_vector_node_desc(cJSON_GetObjectItem(node, "a")),
				.B = build_vector_node_desc(cJSON_GetObjectItem(node, "b")),
				.factor = build_value_node_desc(cJSON_GetObjectItem(node, "f")),
			}
		});
	}
	return NULL;
}

void cr_node_vector_desc_del(struct cr_vector_node *d) {
	if (!d) return;
	switch (d->type) {
		case cr_vec_unknown:
		case cr_vec_constant:
		case cr_vec_normal:
		case cr_vec_uv:
			break;
		case cr_vec_vecmath:
			cr_node_vector_desc_del(d->arg.vecmath.A);
			cr_node_vector_desc_del(d->arg.vecmath.B);
			cr_node_vector_desc_del(d->arg.vecmath.C);
			cr_node_value_desc_del(d->arg.vecmath.f);
			break;
		case cr_vec_mix:
			cr_node_vector_desc_del(d->arg.vec_mix.A);
			cr_node_vector_desc_del(d->arg.vec_mix.B);
			cr_node_value_desc_del(d->arg.vec_mix.factor);
			break;
	}
	free(d);
}

static struct cr_shader_node *bn_alloc(struct cr_shader_node d) {
	struct cr_shader_node *desc = calloc(1, sizeof(*desc));
	memcpy(desc, &d, sizeof(*desc));
	return desc;
}
struct cr_shader_node *build_bsdf_node_desc(const struct cJSON *node) {
	if (!node) return NULL;
	const cJSON *type = cJSON_GetObjectItem(node, "type");
	if (!cJSON_IsString(type)) {
		logr(warning, "No type provided for bsdfNode.\n");
		return NULL;
	}

	if (stringEquals(type->valuestring, "diffuse")) {
		return bn_alloc((struct cr_shader_node){
			.type = cr_bsdf_diffuse,
			.arg.diffuse.color = build_color_node_desc(cJSON_GetObjectItem(node, "color"))
		});
	} else if (stringEquals(type->valuestring, "metal")) {
		return bn_alloc((struct cr_shader_node){
			.type = cr_bsdf_metal,
			.arg.metal = {
				.color = build_color_node_desc(cJSON_GetObjectItem(node, "color")),
				.roughness = build_value_node_desc(cJSON_GetObjectItem(node, "roughness"))
			}
		});
	} else if (stringEquals(type->valuestring, "glass")) {
		return bn_alloc((struct cr_shader_node){
			.type = cr_bsdf_glass,
			.arg.glass = {
				.color = build_color_node_desc(cJSON_GetObjectItem(node, "color")),
				.roughness = build_value_node_desc(cJSON_GetObjectItem(node, "roughness")),
				.IOR = build_value_node_desc(cJSON_GetObjectItem(node, "IOR"))
			}
		});
	} else if (stringEquals(type->valuestring, "plastic")) {
		return bn_alloc((struct cr_shader_node){
			.type = cr_bsdf_plastic,
			.arg.plastic = {
				.color = build_color_node_desc(cJSON_GetObjectItem(node, "color")),
				.roughness = build_value_node_desc(cJSON_GetObjectItem(node, "roughness")),
				.IOR = build_value_node_desc(cJSON_GetObjectItem(node, "IOR"))
			}
		});
	} else if (stringEquals(type->valuestring, "mix")) {
		return bn_alloc((struct cr_shader_node){
			.type = cr_bsdf_mix,
			.arg.mix = {
				.A = build_bsdf_node_desc(cJSON_GetObjectItem(node, "A")),
				.B = build_bsdf_node_desc(cJSON_GetObjectItem(node, "B")),
				.factor = build_value_node_desc(cJSON_GetObjectItem(node, "factor"))
			}
		});
	} else if (stringEquals(type->valuestring, "add")) {
		return bn_alloc((struct cr_shader_node){
			.type = cr_bsdf_add,
			.arg.add = {
				.A = build_bsdf_node_desc(cJSON_GetObjectItem(node, "A")),
				.B = build_bsdf_node_desc(cJSON_GetObjectItem(node, "B")),
			}
		});
	} else if (stringEquals(type->valuestring, "transparent")) {
		return bn_alloc((struct cr_shader_node){
			.type = cr_bsdf_transparent,
			.arg.transparent.color = build_color_node_desc(cJSON_GetObjectItem(node, "color"))
		});
	} else if (stringEquals(type->valuestring, "emissive")) {
		return bn_alloc((struct cr_shader_node){
			.type = cr_bsdf_emissive,
			.arg.emissive = {
				.color = build_color_node_desc(cJSON_GetObjectItem(node, "color")),
				.strength = build_value_node_desc(cJSON_GetObjectItem(node, "strength"))
			}
		});
	} else if (stringEquals(type->valuestring, "translucent")) {
		return bn_alloc((struct cr_shader_node){
			.type = cr_bsdf_translucent,
			.arg.translucent.color = build_color_node_desc(cJSON_GetObjectItem(node, "color"))
		});
	}

	logr(warning, "Failed to parse node. Here's a dump:\n");
	logr(warning, "\n%s\n", cJSON_Print(node));
	logr(warning, "Setting to an obnoxious pink material.\n");
	return NULL;
}

const struct bsdfNode *warningBsdf(const struct node_storage *s) {
	return newMix(s,
				  newDiffuse(s, newConstantTexture(s, warningMaterial().diffuse)),
				  newDiffuse(s, newConstantTexture(s, (struct color){0.2f, 0.2f, 0.2f, 1.0f})),
				  newGrayscaleConverter(s, newCheckerBoardTexture(s, NULL, NULL, newConstantValue(s, 500.0f))));
}

void cr_node_bsdf_desc_del(struct cr_shader_node *d) {
	if (!d) return;
	switch (d->type) {
		case cr_bsdf_unknown:
			return;
		case cr_bsdf_diffuse:
			cr_node_color_desc_del(d->arg.diffuse.color);
			break;
		case cr_bsdf_metal:
			cr_node_color_desc_del(d->arg.metal.color);
			cr_node_value_desc_del(d->arg.metal.roughness);
			break;
		case cr_bsdf_glass:
			cr_node_color_desc_del(d->arg.glass.color);
			cr_node_value_desc_del(d->arg.glass.roughness);
			cr_node_value_desc_del(d->arg.glass.IOR);
			break;
		case cr_bsdf_plastic:
			cr_node_color_desc_del(d->arg.plastic.color);
			cr_node_value_desc_del(d->arg.plastic.roughness);
			cr_node_value_desc_del(d->arg.plastic.IOR);
			break;
		case cr_bsdf_mix:
			cr_node_bsdf_desc_del(d->arg.mix.A);
			cr_node_bsdf_desc_del(d->arg.mix.B);
			cr_node_value_desc_del(d->arg.mix.factor);
			break;
		case cr_bsdf_add:
			cr_node_bsdf_desc_del(d->arg.add.A);
			cr_node_bsdf_desc_del(d->arg.add.B);
			break;
		case cr_bsdf_transparent:
			cr_node_color_desc_del(d->arg.transparent.color);
			break;
		case cr_bsdf_emissive:
			cr_node_color_desc_del(d->arg.emissive.color);
			cr_node_value_desc_del(d->arg.emissive.strength);
			break;
		case cr_bsdf_translucent:
			cr_node_color_desc_del(d->arg.translucent.color);
			break;
	}
	free(d);
}
