//
//  node.h
//  c-ray
//
//  Created by Valtteri Koskivuori on 19/11/2023.
//  Copyright © 2023 Valtteri Koskivuori. All rights reserved.
//

#pragma once

// These are ripped off here:
// https://docs.blender.org/manual/en/latest/render/shader_nodes/converter/math.html
// TODO: Commented ones need to be implemented to reach parity with Cycles. Feel free to do so! :^)
enum cr_math_op {
	Add,
	Subtract,
	Multiply,
	Divide,
	//MultiplyAdd,
	Power,
	Log,
	SquareRoot,
	InvSquareRoot,
	Absolute,
	//Exponent,
	Min,
	Max,
	LessThan,
	GreaterThan,
	Sign,
	Compare,
	//SmoothMin,
	//SmoothMax,
	Round,
	Floor,
	Ceil,
	Truncate,
	Fraction,
	Modulo,
	//Wrap,
	//Snap,
	//PingPong,
	Sine,
	Cosine,
	Tangent,
	//ArcSine,
	//ArcCosine,
	//ArcTangent,
	//ArcTan2,
	//HyperbolicSine,
	//HyperbolicCosine,
	//HyperbolicTangent,
	ToRadians,
	ToDegrees,
};

struct value_node_desc {
	enum value_node_type {
		cr_vn_unknown = 0,
		cr_vn_constant,
		cr_vn_fresnel,
		cr_vn_map_range,
		cr_vn_raylength,
		cr_vn_alpha,
		cr_vn_vec_to_value,
		cr_vn_math,
		cr_vn_grayscale,
	} type;

	union {
		double constant;

		struct fresnel_params {
			struct value_node_desc *IOR;
			struct vector_node_desc *normal;
		} fresnel;

		struct map_range_params {
			struct value_node_desc *input_value;
			struct value_node_desc *from_min;
			struct value_node_desc *from_max;
			struct value_node_desc *to_min;
			struct value_node_desc *to_max;
		} map_range;

		struct alpha_params {
			struct color_node_desc *color;
		} alpha;

		struct vec_to_value_params {
			enum cr_vec_to_value_component {
				X, Y, Z, U, V, F
			} comp;
			struct vector_node_desc *vec;
		} vec_to_value;

		struct math_params {
			struct value_node_desc *A;
			struct value_node_desc *B;
			enum cr_math_op op;
		} math;

		struct grayscale_params {
			struct color_node_desc *color;
		} grayscale;

	} arg;
};

struct cJSON;
struct value_node_desc *build_value_node_desc(const struct cJSON *node);
void cr_node_value_desc_del(struct value_node_desc *d);

// ------

struct color_node_desc {
	enum color_node_type {
		cr_cn_unknown = 0,
		cr_cn_constant,
		cr_cn_image,
		cr_cn_checkerboard,
		cr_cn_blackbody,
		cr_cn_split,
		cr_cn_rgb,
		cr_cn_hsl,
		cr_cn_vec_to_color
	} type;

	union {
		struct cr_color constant;

		struct image_texture_params {
			char *full_path;
			uint8_t options;
		} image;

		struct checkerboard_params {
			struct color_node_desc *a;
			struct color_node_desc *b;
			struct value_node_desc *scale;
		} checkerboard;

		struct blackbody_params {
			struct value_node_desc *degrees;
		} blackbody;

		struct split_params {
			struct value_node_desc *node;
		} split;

		struct rgb_params {
			struct value_node_desc *red;
			struct value_node_desc *green;
			struct value_node_desc *blue;
			// Alpha?
		} rgb;

		struct hsl_params {
			struct value_node_desc *H;
			struct value_node_desc *S;
			struct value_node_desc *L;
		} hsl;

		struct vec_to_color_params {
			struct vector_node_desc *vec;
		} vec_to_color;

	} arg;
};

struct color_node_desc *build_color_node_desc(const struct cJSON *desc);
void cr_node_color_desc_del(struct color_node_desc *d);

// -----

// These are ripped off here:
// https://docs.blender.org/manual/en/latest/render/shader_nodes/converter/vector_math.html
// TODO: Commented ones need to be implemented to reach parity with Cycles. Feel free to do so! :^)
enum cr_vec_op {
	VecAdd,
	VecSubtract,
	VecMultiply,
	VecDivide,
	//VecMultiplyAdd,
	VecCross,
	//VecProject,
	VecReflect,
	VecRefract,
	//VecFaceforward,
	VecDot,
	VecDistance,
	VecLength,
	VecScale,
	VecNormalize,
	VecWrap,
	//VecSnap,
	VecFloor,
	VecCeil,
	VecModulo,
	//VecFraction,
	VecAbs,
	VecMin,
	VecMax,
	VecSin,
	VecCos,
	VecTan,
};

struct vector_node_desc {
	enum cr_vector_node_type {
		cr_vec_unknown = 0,
		cr_vec_constant,
		cr_vec_normal,
		cr_vec_uv,
		cr_vec_vecmath,
		cr_vec_mix,
	} type;

	union {
		struct cr_vector constant;

		struct cr_vecmath_params {
			struct vector_node_desc *A;
			struct vector_node_desc *B;
			struct vector_node_desc *C;
			struct value_node_desc *f;
			enum cr_vec_op op;
		} vecmath;

		struct cr_vec_mix_params {
			struct vector_node_desc *A;
			struct vector_node_desc *B;
			struct value_node_desc *factor;
		} vec_mix;

	} arg;
};

struct vector_node_desc *build_vector_node_desc(const struct cJSON *node);
void cr_node_vector_desc_del(struct vector_node_desc *d);


struct bsdf_node_desc {
	enum cr_bsdf_node_type {
		cr_bsdf_unknown = 0,
		cr_bsdf_diffuse,
		cr_bsdf_metal,
		cr_bsdf_glass,
		cr_bsdf_plastic,
		cr_bsdf_mix,
		cr_bsdf_add,
		cr_bsdf_transparent,
		cr_bsdf_emissive,
		cr_bsdf_translucent,
	} type;

	union {
		struct diffuse_args {
			struct color_node_desc *color;
		} diffuse;

		struct metal_args {
			struct color_node_desc *color;
			struct value_node_desc *roughness;
		} metal;

		struct glass_args {
			struct color_node_desc *color;
			struct value_node_desc *roughness;
			struct value_node_desc *IOR;
		} glass;

		struct plastic_args {
			struct color_node_desc *color;
			struct value_node_desc *roughness;
			struct value_node_desc *IOR;
		} plastic;

		struct mix_args {
			struct bsdf_node_desc *A;
			struct bsdf_node_desc *B;
			struct value_node_desc *factor;
		} mix;

		struct add_args {
			struct bsdf_node_desc *A;
			struct bsdf_node_desc *B;
		} add;

		struct transparent_args {
			struct color_node_desc *color;
		} transparent;

		struct emissive_args {
			struct color_node_desc *color;
			struct value_node_desc *strength;
		} emissive;

		struct translucent_args {
			struct color_node_desc *color;
		} translucent;

	} arg;
};

struct bsdf_node_desc *build_bsdf_node_desc(const struct cJSON *node);
void cr_node_bsdf_desc_del(struct bsdf_node_desc *d);
