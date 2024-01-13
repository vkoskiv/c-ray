//
//  node_parse.c
//  c-ray
//
//  Created by Valtteri Koskivuori on 22/11/2023.
//  Copyright © 2023 Valtteri Koskivuori. All rights reserved.
//

#include <c-ray/c-ray.h>

#include "node_parse.h"
#include "../common/vendored/cJSON.h"

#include "loaders/textureloader.h"
#include "texture.h"
#include "string.h"
#include "logging.h"
#include "color.h"
#include "vector.h"

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

struct cr_value_node *cr_value_node_build(const struct cJSON *node) {
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
			.arg.grayscale.color = cr_color_node_build(node)
		});
	}

	// Note: Fallback for serializer
	if (stringEquals(type->valuestring, "constant")) {
		return vn_alloc((struct cr_value_node){
			.type = cr_vn_constant,
			.arg.constant = cJSON_GetNumberValue(cJSON_GetObjectItem(node, "value"))
		});
	}

	if (stringEquals(type->valuestring, "fresnel")) {
		return vn_alloc((struct cr_value_node){
			.type = cr_vn_fresnel,
			.arg.fresnel = {
				.IOR = cr_value_node_build(cJSON_GetObjectItem(node, "IOR")),
				.normal = cr_vector_node_build(cJSON_GetObjectItem(node, "normal"))
			}
		});
	}
	if (stringEquals(type->valuestring, "map_range")) {
		return vn_alloc((struct cr_value_node){
			.type = cr_vn_map_range,
			.arg.map_range = {
				.input_value = cr_value_node_build(cJSON_GetObjectItem(node, "input")),
				.from_min = cr_value_node_build(cJSON_GetObjectItem(node, "from_min")),
				.from_max = cr_value_node_build(cJSON_GetObjectItem(node, "from_max")),
				.to_min = cr_value_node_build(cJSON_GetObjectItem(node, "to_min")),
				.to_max = cr_value_node_build(cJSON_GetObjectItem(node, "to_max")),
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
			.arg.alpha.color = cr_color_node_build(cJSON_GetObjectItem(node, "color"))
		});
	}
	if (stringEquals(type->valuestring, "vec_to_value")) {
		const cJSON *comp = cJSON_GetObjectItem(node, "component");
		return vn_alloc((struct cr_value_node){
			.type = cr_vn_vec_to_value,
			.arg.vec_to_value = {
				.vec = cr_vector_node_build(cJSON_GetObjectItem(node, "vector")),
				.comp = cJSON_IsNumber(comp) ? (enum cr_vec_to_value_component)comp->valueint : value_node_component(comp) // For serializer
			}
		});
	}
	if (stringEquals(type->valuestring, "math")) {
		const cJSON *op = cJSON_GetObjectItem(node, "op");
		return vn_alloc((struct cr_value_node){
			.type = cr_vn_math,
			.arg.math = {
				.A = cr_value_node_build(cJSON_GetObjectItem(node, "a")),
				.B = cr_value_node_build(cJSON_GetObjectItem(node, "b")),
				.op = cJSON_IsNumber(op) ? (enum cr_math_op)op->valueint : value_node_op(op) // For serializer
			}
		});
	}
	return vn_alloc((struct cr_value_node){
		.type = cr_vn_grayscale,
		.arg.grayscale.color = cr_color_node_build(cJSON_GetObjectItem(node, "color"))
	});
}

void cr_value_node_free(struct cr_value_node *d) {
	if (!d) return;
	switch (d->type) {
		case cr_vn_unknown:
			break;
		case cr_vn_constant:
			break;
		case cr_vn_fresnel:
			cr_value_node_free(d->arg.fresnel.IOR);
			cr_vector_node_free(d->arg.fresnel.normal);
			break;
		case cr_vn_map_range:
			cr_value_node_free(d->arg.map_range.input_value);
			cr_value_node_free(d->arg.map_range.from_min);
			cr_value_node_free(d->arg.map_range.from_max);
			cr_value_node_free(d->arg.map_range.to_min);
			cr_value_node_free(d->arg.map_range.to_max);
			break;
		case cr_vn_raylength:
			break;
		case cr_vn_alpha:
			cr_color_node_free(d->arg.alpha.color);
			break;
		case cr_vn_vec_to_value:
			cr_vector_node_free(d->arg.vec_to_value.vec);
			break;
		case cr_vn_math:
			cr_value_node_free(d->arg.math.A);
			cr_value_node_free(d->arg.math.B);
			break;
		case cr_vn_grayscale:
			cr_color_node_free(d->arg.grayscale.color);
	}
	free(d);
}

static struct cr_color_node *cn_alloc(struct cr_color_node d) {
	struct cr_color_node *desc = calloc(1, sizeof(*desc));
	memcpy(desc, &d, sizeof(*desc));
	return desc;
}

static enum cr_color_mode get_color_mode(char *mode_str) {
	if (stringEquals(mode_str, "rgb")) return cr_mode_rgb;
	if (stringEquals(mode_str, "hsv")) return cr_mode_hsv;
	if (stringEquals(mode_str, "hsl")) return cr_mode_hsl;
	return cr_mode_rgb;
}

static enum cr_interpolation get_interpolation(char *interp_str) {
	if (stringEquals(interp_str, "ease")) return cr_ease;
	if (stringEquals(interp_str, "cardinal")) return cr_cardinal;
	if (stringEquals(interp_str, "linear")) return cr_linear;
	if (stringEquals(interp_str, "b_spline")) return cr_b_spline;
	if (stringEquals(interp_str, "constant")) return cr_constant;
	return cr_linear;
}

static struct ramp_element elem_parse(const struct cJSON *elem_in) {
	struct color c = color_parse(cJSON_GetObjectItem(elem_in, "color"));
	float pos = cJSON_GetNumberValue(cJSON_GetObjectItem(elem_in, "position"));
	return (struct ramp_element){ .color = { c.red, c.green, c.blue, c.alpha }, .position = pos };
}

static struct ramp_element *get_elements(const struct cJSON *arr) {
	size_t count = cJSON_GetArraySize(arr);
	if (!count) return NULL;
	struct ramp_element *elems = calloc(count, sizeof(*elems));
	for (size_t i = 0; i < count; ++i) {
		elems[i] = elem_parse(cJSON_GetArrayItem(arr, i));
	}
	return elems;
}

struct cr_color_node *cr_color_node_build(const struct cJSON *desc) {
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

	// Fallback for serializer, specify options explicitly
	const cJSON *opt = cJSON_GetObjectItem(desc, "options");
	if (cJSON_IsNumber(opt)) {
		options = opt->valueint;
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
		if (stringEquals(type->valuestring, "constant")) {
			struct color c = color_parse(cJSON_GetObjectItem(desc, "color"));
			return cn_alloc((struct cr_color_node){
				.type = cr_cn_constant,
				.arg.constant = { c.red, c.green, c.blue, c.alpha }
			});
		}
		if (!stringEquals(type->valuestring, "image")) {
			if (cJSON_IsString(path)) {
				return cn_alloc((struct cr_color_node){
					.type = cr_cn_image,
					.arg.image.full_path = stringCopy(path->valuestring),
					.arg.image.options = options
				});
			}
		}
		if (stringEquals(type->valuestring, "checkerboard")) {
			return cn_alloc((struct cr_color_node){
				.type = cr_cn_checkerboard,
				.arg.checkerboard = {
					.a = cr_color_node_build(cJSON_GetObjectItem(desc, "color1")),
					.b = cr_color_node_build(cJSON_GetObjectItem(desc, "color2")),
					.scale = cr_value_node_build(cJSON_GetObjectItem(desc, "scale"))
				}
			});
		}
		if (stringEquals(type->valuestring, "blackbody")) {
			// FIXME: valuenode
			return cn_alloc((struct cr_color_node){
				.type = cr_cn_blackbody,
				.arg.blackbody.degrees = cr_value_node_build(cJSON_GetObjectItem(desc, "degrees"))
			});
		}
		if (stringEquals(type->valuestring, "split")) {
			return cn_alloc((struct cr_color_node){
				.type = cr_cn_split,
				.arg.split.node = cr_value_node_build(cJSON_GetObjectItem(desc, "constant"))
			});
		}
		if (stringEquals(type->valuestring, "rgb")) {
			return cn_alloc((struct cr_color_node){
				.type = cr_cn_rgb,
				.arg.rgb = {
					.red = cr_value_node_build(cJSON_GetObjectItem(desc, "r")),
					.green = cr_value_node_build(cJSON_GetObjectItem(desc, "g")),
					.blue = cr_value_node_build(cJSON_GetObjectItem(desc, "b")),
				}
			});
		}
		if (stringEquals(type->valuestring, "hsl")) {
			return cn_alloc((struct cr_color_node){
				.type = cr_cn_hsl,
				.arg.hsl = {
					.H = cr_value_node_build(cJSON_GetObjectItem(desc, "h")),
					.S = cr_value_node_build(cJSON_GetObjectItem(desc, "s")),
					.L = cr_value_node_build(cJSON_GetObjectItem(desc, "l")),
				}
			});
		}
		if (stringEquals(type->valuestring, "hsv")) {
			return cn_alloc((struct cr_color_node){
				.type = cr_cn_hsv,
				.arg.hsv = {
					.H = cr_value_node_build(cJSON_GetObjectItem(desc, "h")),
					.S = cr_value_node_build(cJSON_GetObjectItem(desc, "s")),
					.V = cr_value_node_build(cJSON_GetObjectItem(desc, "v")),
				}
			});
		}
		if (stringEquals(type->valuestring, "hsv_tform")) {
			return cn_alloc((struct cr_color_node){
				.type = cr_cn_hsv_tform,
				.arg.hsv_tform = {
					.tex = cr_color_node_build(cJSON_GetObjectItem(desc, "tex")),
					.H = cr_value_node_build(cJSON_GetObjectItem(desc, "h")),
					.S = cr_value_node_build(cJSON_GetObjectItem(desc, "s")),
					.V = cr_value_node_build(cJSON_GetObjectItem(desc, "v")),
					.f = cr_value_node_build(cJSON_GetObjectItem(desc, "f")),
				}
			});
		}
		if (stringEquals(type->valuestring, "to_color")) {
			return cn_alloc((struct cr_color_node){
				.type = cr_cn_vec_to_color,
				.arg.vec_to_color.vec = cr_vector_node_build(cJSON_GetObjectItem(desc, "vector"))
			});
		}
		if (stringEquals(type->valuestring, "gradient")) {
			return cn_alloc((struct cr_color_node){
				.type = cr_cn_gradient,
				.arg.gradient = {
					.a = cr_color_node_build(cJSON_GetObjectItem(desc, "down")),
					.b = cr_color_node_build(cJSON_GetObjectItem(desc, "up")),
				}
			});
		}
		if (stringEquals(type->valuestring, "color_mix")) {
			return cn_alloc((struct cr_color_node){
				.type = cr_cn_color_mix,
				.arg.color_mix = {
					.a = cr_color_node_build(cJSON_GetObjectItem(desc, "a")),
					.b = cr_color_node_build(cJSON_GetObjectItem(desc, "b")),
					.factor = cr_value_node_build(cJSON_GetObjectItem(desc, "factor"))
				}
			});
		}
		if (stringEquals(type->valuestring, "color_ramp")) {
			return cn_alloc((struct cr_color_node){
				.type = cr_cn_color_ramp,
				.arg.color_ramp = {
					.factor = cr_value_node_build(cJSON_GetObjectItem(desc, "factor")),
					.color_mode = get_color_mode(cJSON_GetStringValue(cJSON_GetObjectItem(desc, "color_mode"))),
					.interpolation = get_interpolation(cJSON_GetStringValue(cJSON_GetObjectItem(desc, "interpolation"))),
					.elements = get_elements(cJSON_GetObjectItem(desc, "elements")),
					.element_count = cJSON_GetArraySize(cJSON_GetObjectItem(desc, "elements"))
				}
			});
		}
	}

	logr(warning, "Failed to parse textureNode. Here's a dump:\n");
	logr(warning, "\n%s\n", cJSON_Print(desc));
	logr(warning, "Setting to an obnoxious pink material.\n");
	return NULL;

}

void cr_color_node_free(struct cr_color_node *d) {
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
			cr_color_node_free(d->arg.checkerboard.a);
			cr_color_node_free(d->arg.checkerboard.b);
			cr_value_node_free(d->arg.checkerboard.scale);
			break;
		case cr_cn_blackbody:
			cr_value_node_free(d->arg.blackbody.degrees);
			break;
		case cr_cn_split:
			cr_value_node_free(d->arg.split.node);
			break;
		case cr_cn_rgb:
			cr_value_node_free(d->arg.rgb.red);
			cr_value_node_free(d->arg.rgb.green);
			cr_value_node_free(d->arg.rgb.blue);
			break;
		case cr_cn_hsl:
			cr_value_node_free(d->arg.hsl.H);
			cr_value_node_free(d->arg.hsl.S);
			cr_value_node_free(d->arg.hsl.L);
			break;
		case cr_cn_hsv:
			cr_value_node_free(d->arg.hsv.H);
			cr_value_node_free(d->arg.hsv.S);
			cr_value_node_free(d->arg.hsv.V);
			break;
		case cr_cn_hsv_tform:
			cr_color_node_free(d->arg.hsv_tform.tex);
			cr_value_node_free(d->arg.hsv_tform.H);
			cr_value_node_free(d->arg.hsv_tform.S);
			cr_value_node_free(d->arg.hsv_tform.V);
			cr_value_node_free(d->arg.hsv_tform.f);
			break;
		case cr_cn_vec_to_color:
			cr_vector_node_free(d->arg.vec_to_color.vec);
			break;
		case cr_cn_gradient:
			cr_color_node_free(d->arg.gradient.a);
			cr_color_node_free(d->arg.gradient.b);
			break;
		case cr_cn_color_mix:
			cr_color_node_free(d->arg.color_mix.a);
			cr_color_node_free(d->arg.color_mix.b);
			cr_value_node_free(d->arg.color_mix.factor);
			break;
		case cr_cn_color_ramp:
			cr_value_node_free(d->arg.color_ramp.factor);
			if (d->arg.color_ramp.elements)
				free(d->arg.color_ramp.elements);
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

struct cr_vector_node *cr_vector_node_build(const struct cJSON *node) {
	if (!node) return NULL;
	if (cJSON_IsNumber(node)) {
		return vecn_alloc((struct cr_vector_node){
			.type = cr_vec_constant,
			.arg.constant.x = node->valuedouble
		});
	}
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

	// Note: Fallback for serializer
	if (stringEquals(type->valuestring, "constant")) {
		struct vector v = parseVector(cJSON_GetObjectItem(node, "vec"));
		return vecn_alloc((struct cr_vector_node){
			.type = cr_vec_constant,
			.arg.constant = { v.x, v.y, v.z }
		});
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
		const cJSON *op = cJSON_GetObjectItem(node, "op");
		return vecn_alloc((struct cr_vector_node){
			.type = cr_vec_vecmath,
			.arg.vecmath = {
				.A = cr_vector_node_build(cJSON_GetObjectItem(node, "a")),
				.B = cr_vector_node_build(cJSON_GetObjectItem(node, "b")),
				.C = cr_vector_node_build(cJSON_GetObjectItem(node, "c")),
				.f = cr_value_node_build(cJSON_GetObjectItem(node, "f")),
				.op = cJSON_IsNumber(op) ? (enum cr_vec_op)op->valueint : parseVectorOp(op) // Note: Fallback for serializer
			}
		});
	}
	if (stringEquals(type->valuestring, "mix")) {
		return vecn_alloc((struct cr_vector_node){
			.type = cr_vec_mix,
			.arg.vec_mix = {
				.A = cr_vector_node_build(cJSON_GetObjectItem(node, "a")),
				.B = cr_vector_node_build(cJSON_GetObjectItem(node, "b")),
				.factor = cr_value_node_build(cJSON_GetObjectItem(node, "f")),
			}
		});
	}
	return NULL;
}

void cr_vector_node_free(struct cr_vector_node *d) {
	if (!d) return;
	switch (d->type) {
		case cr_vec_unknown:
		case cr_vec_constant:
		case cr_vec_normal:
		case cr_vec_uv:
			break;
		case cr_vec_vecmath:
			cr_vector_node_free(d->arg.vecmath.A);
			cr_vector_node_free(d->arg.vecmath.B);
			cr_vector_node_free(d->arg.vecmath.C);
			cr_value_node_free(d->arg.vecmath.f);
			break;
		case cr_vec_mix:
			cr_vector_node_free(d->arg.vec_mix.A);
			cr_vector_node_free(d->arg.vec_mix.B);
			cr_value_node_free(d->arg.vec_mix.factor);
			break;
	}
	free(d);
}

static struct cr_shader_node *bn_alloc(struct cr_shader_node d) {
	struct cr_shader_node *desc = calloc(1, sizeof(*desc));
	memcpy(desc, &d, sizeof(*desc));
	return desc;
}

struct cr_shader_node *cr_shader_node_build(const struct cJSON *node) {
	if (!node) return NULL;
	const cJSON *type = cJSON_GetObjectItem(node, "type");
	if (!cJSON_IsString(type)) {
		logr(warning, "No type provided for bsdfNode.\n");
		return NULL;
	}

	if (stringEquals(type->valuestring, "diffuse")) {
		return bn_alloc((struct cr_shader_node){
			.type = cr_bsdf_diffuse,
			.arg.diffuse.color = cr_color_node_build(cJSON_GetObjectItem(node, "color"))
		});
	} else if (stringEquals(type->valuestring, "metal")) {
		return bn_alloc((struct cr_shader_node){
			.type = cr_bsdf_metal,
			.arg.metal = {
				.color = cr_color_node_build(cJSON_GetObjectItem(node, "color")),
				.roughness = cr_value_node_build(cJSON_GetObjectItem(node, "roughness"))
			}
		});
	} else if (stringEquals(type->valuestring, "glass")) {
		return bn_alloc((struct cr_shader_node){
			.type = cr_bsdf_glass,
			.arg.glass = {
				.color = cr_color_node_build(cJSON_GetObjectItem(node, "color")),
				.roughness = cr_value_node_build(cJSON_GetObjectItem(node, "roughness")),
				.IOR = cr_value_node_build(cJSON_GetObjectItem(node, "IOR"))
			}
		});
	} else if (stringEquals(type->valuestring, "plastic")) {
		return bn_alloc((struct cr_shader_node){
			.type = cr_bsdf_plastic,
			.arg.plastic = {
				.color = cr_color_node_build(cJSON_GetObjectItem(node, "color")),
				.roughness = cr_value_node_build(cJSON_GetObjectItem(node, "roughness")),
				.IOR = cr_value_node_build(cJSON_GetObjectItem(node, "IOR"))
			}
		});
	} else if (stringEquals(type->valuestring, "mix")) {
		return bn_alloc((struct cr_shader_node){
			.type = cr_bsdf_mix,
			.arg.mix = {
				.A = cr_shader_node_build(cJSON_GetObjectItem(node, "A")),
				.B = cr_shader_node_build(cJSON_GetObjectItem(node, "B")),
				.factor = cr_value_node_build(cJSON_GetObjectItem(node, "factor"))
			}
		});
	} else if (stringEquals(type->valuestring, "add")) {
		return bn_alloc((struct cr_shader_node){
			.type = cr_bsdf_add,
			.arg.add = {
				.A = cr_shader_node_build(cJSON_GetObjectItem(node, "A")),
				.B = cr_shader_node_build(cJSON_GetObjectItem(node, "B")),
			}
		});
	} else if (stringEquals(type->valuestring, "transparent")) {
		return bn_alloc((struct cr_shader_node){
			.type = cr_bsdf_transparent,
			.arg.transparent.color = cr_color_node_build(cJSON_GetObjectItem(node, "color"))
		});
	} else if (stringEquals(type->valuestring, "emissive")) {
		return bn_alloc((struct cr_shader_node){
			.type = cr_bsdf_emissive,
			.arg.emissive = {
				.color = cr_color_node_build(cJSON_GetObjectItem(node, "color")),
				.strength = cr_value_node_build(cJSON_GetObjectItem(node, "strength"))
			}
		});
	} else if (stringEquals(type->valuestring, "translucent")) {
		return bn_alloc((struct cr_shader_node){
			.type = cr_bsdf_translucent,
			.arg.translucent.color = cr_color_node_build(cJSON_GetObjectItem(node, "color"))
		});
	} else if (stringEquals(type->valuestring, "background")) {
		// Note: Fallback for serializer
		struct cr_color_node *color = NULL;
		const cJSON *color_in = cJSON_GetObjectItem(node, "color");
		if (cJSON_IsObject(color_in)) {
			// This was generated by the internal serializer.
			color = cr_color_node_build(color_in);
		} else {
			// Normal (ugly) encoding from json
			struct cr_color_node *hdr = NULL;
			struct cr_color_node *gradient = NULL;
			// Special cases for scene.ambientColor
			const cJSON *hdr_in = cJSON_GetObjectItem(node, "hdr");
			if (cJSON_IsString(hdr_in)) {
				hdr = cn_alloc((struct cr_color_node){
					.type = cr_cn_image,
					.arg.image.full_path = stringCopy(hdr_in->valuestring),
					.arg.image.options = 0 // TODO: Options?
				});
			}
			const cJSON *down = cJSON_GetObjectItem(node, "down");
			const cJSON *up = cJSON_GetObjectItem(node, "up");
			if (!hdr && cJSON_IsObject(down) && cJSON_IsObject(up)) {
				gradient = cn_alloc((struct cr_color_node){
					.type = cr_cn_gradient,
					.arg.gradient = {
						.a = cr_color_node_build(down),
						.b = cr_color_node_build(up),
					}
				});
			}
			color = hdr ? hdr : gradient;
		}

		const cJSON *strength = cJSON_GetObjectItem(node, "strength");
		const cJSON *offset = cJSON_GetObjectItem(node, "offset");
		return bn_alloc((struct cr_shader_node){
			.type = cr_bsdf_background,
			.arg.background = {
				.color = color,
				.pose = cr_vector_node_build(offset),
				.strength = cr_value_node_build(strength),
			}
		});
	}

	logr(warning, "Failed to parse node. Here's a dump:\n");
	logr(warning, "\n%s\n", cJSON_Print(node));
	logr(warning, "Setting to an obnoxious pink material.\n");
	return NULL;
}

void cr_shader_node_free(struct cr_shader_node *d) {
	if (!d) return;
	switch (d->type) {
		case cr_bsdf_unknown:
			return;
		case cr_bsdf_diffuse:
			cr_color_node_free(d->arg.diffuse.color);
			break;
		case cr_bsdf_metal:
			cr_color_node_free(d->arg.metal.color);
			cr_value_node_free(d->arg.metal.roughness);
			break;
		case cr_bsdf_glass:
			cr_color_node_free(d->arg.glass.color);
			cr_value_node_free(d->arg.glass.roughness);
			cr_value_node_free(d->arg.glass.IOR);
			break;
		case cr_bsdf_plastic:
			cr_color_node_free(d->arg.plastic.color);
			cr_value_node_free(d->arg.plastic.roughness);
			cr_value_node_free(d->arg.plastic.IOR);
			break;
		case cr_bsdf_mix:
			cr_shader_node_free(d->arg.mix.A);
			cr_shader_node_free(d->arg.mix.B);
			cr_value_node_free(d->arg.mix.factor);
			break;
		case cr_bsdf_add:
			cr_shader_node_free(d->arg.add.A);
			cr_shader_node_free(d->arg.add.B);
			break;
		case cr_bsdf_transparent:
			cr_color_node_free(d->arg.transparent.color);
			break;
		case cr_bsdf_emissive:
			cr_color_node_free(d->arg.emissive.color);
			cr_value_node_free(d->arg.emissive.strength);
			break;
		case cr_bsdf_translucent:
			cr_color_node_free(d->arg.translucent.color);
			break;
		case cr_bsdf_background:
			cr_color_node_free(d->arg.background.color);
			cr_vector_node_free(d->arg.background.pose);
			cr_value_node_free(d->arg.background.strength);
			break;
	}
	free(d);
}

struct color color_parse(const cJSON *data) {
	if (cJSON_IsArray(data)) {
		const float r = cJSON_IsNumber(cJSON_GetArrayItem(data, 0)) ? (float)cJSON_GetArrayItem(data, 0)->valuedouble : 0.0f;
		const float g = cJSON_IsNumber(cJSON_GetArrayItem(data, 1)) ? (float)cJSON_GetArrayItem(data, 1)->valuedouble : 0.0f;
		const float b = cJSON_IsNumber(cJSON_GetArrayItem(data, 2)) ? (float)cJSON_GetArrayItem(data, 2)->valuedouble : 0.0f;
		const float a = cJSON_IsNumber(cJSON_GetArrayItem(data, 3)) ? (float)cJSON_GetArrayItem(data, 3)->valuedouble : 1.0f;
		return (struct color){ r, g, b, a };
	}

	ASSERT(cJSON_IsObject(data));

	const cJSON *kelvin = cJSON_GetObjectItem(data, "blackbody");
	if (cJSON_IsNumber(kelvin)) return colorForKelvin(kelvin->valuedouble);

	const cJSON *H = cJSON_GetObjectItem(data, "h");
	const cJSON *S = cJSON_GetObjectItem(data, "s");
	const cJSON *L = cJSON_GetObjectItem(data, "l");
	const cJSON *V = cJSON_GetObjectItem(data, "v");

	if (cJSON_IsNumber(H) && cJSON_IsNumber(S) && cJSON_IsNumber(L)) {
		return hsl_to_rgb((struct hsl){ H->valuedouble, S->valuedouble, L->valuedouble });
	}

	if (cJSON_IsNumber(H) && cJSON_IsNumber(S) && cJSON_IsNumber(V)) {
		return hsv_to_rgb((struct hsv){ H->valuedouble, S->valuedouble, V->valuedouble });
	}
	
	const cJSON *R = cJSON_GetObjectItem(data, "r");
	const cJSON *G = cJSON_GetObjectItem(data, "g");
	const cJSON *B = cJSON_GetObjectItem(data, "b");
	const cJSON *A = cJSON_GetObjectItem(data, "a");

	return (struct color){
		cJSON_IsNumber(R) ? (float)R->valuedouble : 0.0f,
		cJSON_IsNumber(G) ? (float)G->valuedouble : 0.0f,
		cJSON_IsNumber(B) ? (float)B->valuedouble : 0.0f,
		cJSON_IsNumber(A) ? (float)A->valuedouble : 1.0f,
	};
}

