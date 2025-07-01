# Convert Cycles nodegraphs to c-ray nodegraphs

from . shader import *
from . color import *
from . value import *
from . vector import *

# No clue why, * apparently still skips some imports
from . vector import _vector_op
from . value import _math_op
from . value import _component

def convert_background(nt):
	if not nt:
		print("No background shader set, bailing out")
		return None
	if len(nt.nodes) < 1:
		print("No nodes found for background, bailing out")
		return None
	if not 'World Output' in nt.nodes:
		print("No World Output node found in tree for background, bailing out")
		return None
	root = nt.nodes['World Output']
	return parse_node(root.inputs['Surface'].links[0].from_node)
		

def convert_node_tree(bl_depsgraph, name, nt):
	if not nt:
		print("No node tree in material {}, bailing out".format(name))
		return None
	if len(nt.nodes) < 1:
		print("No nodes found for material {}, bailing out".format(name))
		return None
	if not 'Material Output' in nt.nodes:
		print("No Material Output node found in tree for {}, bailing out".format(name))
		return None
	root = nt.nodes['Material Output']
	if not root.inputs['Surface'].is_linked:
		return NodeShaderDiffuse(NodeColorConstant(cr_color(0.0, 0.0, 0.0, 1.0)))
	return parse_node(root.inputs['Surface'].links[0].from_node)

warning_color = NodeColorCheckerboard(NodeColorConstant(cr_color(1.0, 0.0, 0.0, 0.0)), NodeColorConstant(cr_color(1.0, 1.0, 1.0, 1.0)), NodeValueConstant(100.0))

# Blender's mix node (ShaderNodeMix) is internally shared among different types.
# Confusingly, the different inputs share the same names, so here are the input indices, names and types:
# 0:Factor --> NodeSocketFloatFactor
# 1:Factor --> NodeSocketVector
# 2:A --> NodeSocketFloat
# 3:B --> NodeSocketFloat
# 4:A --> NodeSocketVector
# 5:B --> NodeSocketVector
# 6:A --> NodeSocketColor
# 7:B --> NodeSocketColor
# 8:A --> NodeSocketRotation
# 9:B --> NodeSocketRotation

def parse_color(input, group_inputs):
	match input.bl_idname:
		case 'ShaderNodeGroup':
			return parse_subtree(input, ['NodeSocketColor'], parse_color, group_inputs, warning_color)
		case 'NodeReroute':
			return parse_color(input.inputs[0], group_inputs)
		case 'NodeSocketColor':
			if input.is_linked:
				if input.links[0].from_node.bl_idname == 'NodeGroupInput':
					return group_inputs[input.links[0].from_socket.name]
				return parse_color(input.links[0].from_node, group_inputs)
			vals = input.default_value
			return NodeColorConstant(cr_color(vals[0], vals[1], vals[2], vals[3]))
		case 'NodeSocketVector':
			if input.is_linked:
				if input.links[0].from_node.bl_idname == 'NodeGroupInput':
					return group_inputs[input.links[0].from_socket.name]
				return NodeColorVecToColor(parse_vector(input.links[0].from_node, group_inputs))
			vec = input.default_value
			return NodeColorVecToColor(NodeVectorConstant(vec))
		case 'ShaderNodeTexImage':
			if input.image is None:
				print("No image set in blender image texture {}".format(input.name))
				return warning_color
			path = input.image.filepath_from_user()
			return NodeColorImageTexture(path, 0)
		case 'ShaderNodeTexChecker':
			color1 = parse_color(input.inputs['Color1'], group_inputs)
			color2 = parse_color(input.inputs['Color2'], group_inputs)
			scale = parse_value(input.inputs['Scale'], group_inputs)
			return NodeColorCheckerboard(color1, color2, scale)
		case 'ShaderNodeTexEnvironment':
			if input.image is None:
				print("No image set in blender environment texture {}".format(input.name))
				return warning_color
			path = input.image.filepath_from_user()
			return NodeColorImageTexture(path, 0)
		case 'ShaderNodeRGB':
			color = input.outputs[0].default_value
			return NodeColorConstant(cr_color(color[0], color[1], color[2], color[3]))
		case 'ShaderNodeMix':
			factor = parse_value(input.inputs[0], group_inputs)
			# Hack - piggyback off vec_mix instead of implementing one for color
			if input.inputs[4].is_linked and input.inputs[5].is_linked:
				return NodeColorVecToColor(parse_vector(input, group_inputs))

			a = parse_color(input.inputs[6], group_inputs)
			b = parse_color(input.inputs[7], group_inputs)
			return NodeColorMix(a, b, factor)
		case 'ShaderNodeBlackbody':
			degrees = parse_value(input.inputs[0], group_inputs)
			return NodeColorBlackbody(degrees)
		case 'ShaderNodeHueSaturation':
			color = parse_color(input.inputs['Color'], group_inputs)
			hue = parse_value(input.inputs['Hue'], group_inputs)
			sat = parse_value(input.inputs['Saturation'], group_inputs)
			val = parse_value(input.inputs['Value'], group_inputs)
			fac = parse_value(input.inputs['Fac'], group_inputs)
			return NodeColorHSVTransform(color, hue, sat, val, fac)
		case 'ShaderNodeValToRGB':
			# Confusing name, this is the Color Ramp node.
			factor = parse_value(input.inputs['Fac'], group_inputs)
			color = input.color
			ramp = input.color_ramp
			# ramp has: hue_interpolation, interpolation, color_mode
			elements = []
			for element in ramp.elements:
				blc = element.color
				color = cr_color(blc[0], blc[1], blc[2], blc[3])
				elements.append(ramp_element(color, element.position))
			# element has: position, color, alpha
			def match_interpolation(bl_mode):
				match bl_mode:
					case 'EASE':
						return interpolation.ease
					case 'CARDINAL':
						return interpolation.cardinal
					case 'LINEAR':
						return interpolation.linear
					case 'B_SPLINE':
						return interpolation.b_spline
					case 'CONSTANT':
						return interpolation.constant
					case _:
						print("cm WTF: {}".format(bl_mode))
			def match_color_mode(bl_int):
				match bl_int:
					case 'RGB':
						return color_mode.mode_rgb
					case 'HSV':
						return color_mode.mode_hsv
					case 'HSL':
						return color_mode.mode_hsl
					case _:
						print("interp WTF: {}".format(bl_int))
			return NodeColorRamp(factor, match_color_mode(ramp.color_mode), match_interpolation(ramp.interpolation), elements)
		# case 'ShaderNodeCombineRGB':
		# 	return warning_color
		# case 'ShaderNodeCombineHSL':
		# 	return warning_color
		# vec_to_color is implicit in blender nodes
		# case 'ShaderNodeTexGradient':
		# 	return warning_color
		case _:
			print("Unknown color node of type {}, maybe fix.".format(input.bl_idname))
			return warning_color

math_ops = {
	'ADD': _math_op.Add,
	'SUBTRACT': _math_op.Subtract,
	'MULTIPLY': _math_op.Multiply,
	'DIVIDE': _math_op.Divide,
	# 'MULTIPLY_ADD'
	'POWER': _math_op.Power,
	'LOGARITHM': _math_op.Log,
	'SQRT': _math_op.SquareRoot,
	'INVERSE_SQRT': _math_op.InvSquareRoot,
	'ABSOLUTE': _math_op.Absolute,
	# 'EXPONENT'
	'MINIMUM': _math_op.Min,
	'MAXIMUM': _math_op.Max,
	'LESS_THAN': _math_op.LessThan,
	'GREATER_THAN': _math_op.GreaterThan,
	'SIGN': _math_op.Sign,
	'COMPARE': _math_op.Compare, # FIXME: Ignoring c, using hard-coded epsilon
	# 'SMOOTH_MIN'
	# 'SMOOTH_MAX'
	'ROUND': _math_op.Round,
	'FLOOR': _math_op.Floor,
	'CEIL': _math_op.Ceil,
	'TRUNC': _math_op.Truncate,
	'FRACT': _math_op.Fraction,
	'MODULO': _math_op.Modulo,
	# 'FLOORED_MODULO'
	# 'WRAP'
	# 'SNAP'
	# 'PINGPONG'
	'SINE': _math_op.Sine,
	'COSINE': _math_op.Cosine,
	'TANGENT': _math_op.Tangent,
	# 'ARCSINE'
	# 'ARCCOSINE'
	# 'ARCTANGENT'
	# 'ARCTAN2'
	# 'SINH'
	# 'COSH'
	# 'TANH'
	'RADIANS': _math_op.ToRadians,
	'DEGREES': _math_op.ToDegrees,
}

def map_math_op(bl_op):
	if bl_op in math_ops:
		return math_ops[bl_op]
	else:
		print("Unknown math op {}, defaulting to ADD".format(bl_op))
		return _math_op.Add

ray_queries = {
	'Is Camera Ray': light_path_query.is_camera_ray,
	'Is Shadow Ray': light_path_query.is_shadow_ray,
	'Is Diffuse Ray': light_path_query.is_diffuse_ray,
	'Is Glossy Ray': light_path_query.is_glossy_ray,
	'Is Singular Ray': light_path_query.is_singular_ray,
	'Is Reflection Ray': light_path_query.is_reflection_ray,
	'Is Transmission Ray': light_path_query.is_transmission_ray,
	'Ray Length': light_path_query.ray_length,
}

def match_query(name):
	if name in ray_queries:
		return ray_queries[name]
	else:
		print(f"Unknown light path query '{name}', defaulting to 'Ray Length'")
		return light_path_query.ray_length

def parse_value(input, group_inputs):
	match input.bl_idname:
		case 'ShaderNodeGroup':
			return parse_subtree(input, ['NodeSocketFloat', 'NodeSocketfloatFactor'], parse_value, group_inputs, NodeValueConstant(1.0))
		case 'NodeReroute':
			return parse_value(input.inputs[0], group_inputs)
		case 'NodeSocketColor':
			if input.is_linked:
				if input.links[0].from_node.bl_idname == 'NodeGroupInput':
					return group_inputs[input.links[0].from_socket.name]
				return NodeValueGrayscale(parse_color(input.links[0].from_node, group_inputs))
			vals = input.default_value
			return NodeValueGrayscale(NodeColorConstant(cr_color(vals[0], vals[1], vals[2], vals[3])))
		case 'NodeSocketFloat':
			if input.is_linked:
				if input.links[0].from_node.bl_idname == 'NodeGroupInput':
					return group_inputs[input.links[0].from_socket.name]
				if input.links[0].from_node.bl_idname == 'ShaderNodeLightPath':
					socket_name = input.links[0].from_socket.name
					return NodeValueLightPath(match_query(socket_name))
				return parse_value(input.links[0].from_node, group_inputs)
			return NodeValueConstant(input.default_value)
		case 'NodeSocketFloatFactor':
			# note: same as Float, but range is [0,1]
			if input.is_linked:
				if input.links[0].from_node.bl_idname == 'NodeGroupInput':
					return group_inputs[input.links[0].from_socket.name]
				if input.links[0].from_node.bl_idname == 'ShaderNodeLightPath':
					socket_name = input.links[0].from_socket.name
					return NodeValueLightPath(match_query(socket_name))
				return parse_value(input.links[0].from_node, group_inputs)
			return NodeValueConstant(input.default_value)
		case 'ShaderNodeValue':
			value = input.outputs[0].default_value
			return NodeValueConstant(value)
		case 'ShaderNodeMath':
			a = parse_value(input.inputs[0], group_inputs)
			b = parse_value(input.inputs[1], group_inputs)
			# c = parse_value(input.inputs[2], group_inputs)
			op = map_math_op(input.operation)
			return NodeValueMath(a, b, op)
		case 'ShaderNodeVectorMath':
			vec = parse_vector(input, group_inputs)
			return NodeValueVecToValue(_component.F, vec)
		case 'ShaderNodeMix':
			factor = parse_value(input.inputs[0], group_inputs)
			a = parse_value(input.inputs[2], group_inputs)
			b = parse_value(input.inputs[3], group_inputs)
			# Hack - We should implement dedicated color and value mix
			color = NodeColorMix(NodeColorSplit(a), NodeColorSplit(b), factor)
			return NodeValueGrayscale(color)
		case 'ShaderNodeFresnel':
			ior = parse_value(input.inputs[0], group_inputs)
			normal = parse_vector(input.inputs[1], group_inputs)
			return NodeValueFresnel(ior, normal)
		case 'ShaderNodeValToRGB':
			color = parse_color(input, group_inputs)
			return NodeValueGrayscale(color)
		case 'ShaderNodeLayerWeight':
			# FIXME: Ignoring blend
			blend = parse_value(input.inputs[0], group_inputs)
			normal = parse_vector(input.inputs[1], group_inputs)
			# FIXME: If both are linked, will always take this first one
			if input.outputs['Fresnel'].is_linked:
				return NodeValueFresnel(NodeValueConstant(1.0), normal)
			if input.outputs['Facing'].is_linked:
				# TODO
				return NodeValueConstant(0.0)
		case _:
			print("Unknown value node of type {}, maybe fix.".format(input.bl_idname))
			return NodeValueConstant(1.0)

# From here: https://docs.blender.org/api/current/bpy_types_enum_items/node_vec_math_items.html#rna-enum-node-vec-math-items
# Commented ones aren't implemented yet, and will emit a notice to stdout
vec_ops = {
	'ADD': _vector_op.Add,
	'SUBTRACT': _vector_op.Subtract,
	'MULTIPLY': _vector_op.Multiply,
	'DIVIDE': _vector_op.Divide,
	# 'MULTIPLY_ADD'
	'CROSS_PRODUCT': _vector_op.Cross,
	# 'PROJECT'
	'REFLECT': _vector_op.Reflect,
	'REFRACT': _vector_op.Refract,
	# 'FACEFORWARD'
	'DOT_PRODUCT': _vector_op.Dot,
	'DISTANCE': _vector_op.Distance,
	'LENGTH': _vector_op.Length,
	'SCALE': _vector_op.Scale,
	'NORMALIZE': _vector_op.Normalize,
	'ABSOLUTE': _vector_op.Abs,
	'MINIMUM': _vector_op.Min,
	'MAXIMUM': _vector_op.Max,
	'FLOOR': _vector_op.Floor,
	'CEIL': _vector_op.Ceil,
	# 'FRACTION'
	'MODULO': _vector_op.Modulo,
	'WRAP': _vector_op.Wrap,
	# 'SNAP'
	'SINE': _vector_op.Sin,
	'COSINE': _vector_op.Cos,
	'TANGENT': _vector_op.Tan,
}

def map_vec_op(bl_op):
	if bl_op in vec_ops:
		return vec_ops[bl_op]
	else:
		print(f"Unknown vector op {bl_op}, defaulting to ADD")
		return _vector_op.Add

zero_vec = NodeVectorConstant(cr_vector(0.0, 0.0, 0.0))

def parse_vector(input, group_inputs):
	match input.bl_idname:
		case 'ShaderNodeGroup':
			return parse_subtree(input, ['NodeSocketVector'], parse_vector, group_inputs, zero_vec)
		case 'NodeReroute':
			return parse_vector(input.inputs[0], group_inputs)
		case 'NodeSocketColor':
			if input.is_linked:
				if input.links[0].from_node.bl_idname == 'NodeGroupInput':
					return group_inputs[input.links[0].from_socket.name]
				return NodeVectorFromColor(parse_color(input.links[0].from_node, group_inputs))
			vals = input.default_value
			return NodeVectorFromColor(NodeColorConstant(cr_color(vals[0], vals[1], vals[2], vals[3])))
		case 'NodeSocketVector':
			if input.is_linked:
				if input.links[0].from_node.bl_idname == 'NodeGroupInput':
					return group_inputs[input.links[0].from_socket.name]
				return parse_vector(input.links[0].from_node, group_inputs)
			vec = input.default_value
			return NodeVectorConstant(cr_vector(vec[0], vec[1], vec[2]))
		case 'ShaderNodeMix':
			a = parse_vector(input.inputs[4], group_inputs)
			b = parse_vector(input.inputs[5], group_inputs)
			factor = parse_value(input.inputs[0], group_inputs)
			return NodeVectorVecMix(a, b, factor)
		case 'ShaderNodeVectorMath':
			a = parse_vector(input.inputs[0], group_inputs)
			b = parse_vector(input.inputs[1], group_inputs)
			c = parse_vector(input.inputs[2], group_inputs)
			f = parse_value(input.inputs[3], group_inputs)
			op = map_vec_op(input.operation)
			return NodeVectorVecMath(a, b, c, f, op)
		case 'ShaderNodeTexCoord':
			# FIXME: I doubt this will work
			if input.outputs['UV'].is_linked:
				return NodeVectorUV()
			if input.outputs['Normal'].is_linked:
				return NodeVectorNormal()
			print("Unsupported ShaderNodeTexCoord, here's a dump:")
			for output in input.outputs:
				print("{}: {}".format(output.name, output.is_linked))
		case _:
			print("Unknown vector node of type {}, maybe fix.".format(input.bl_idname))
			return zero_vec

warning_shader = NodeShaderDiffuse(warning_color)

def parse_subtree(input, out_sock_types, sub_parser, group_inputs, default=None):
	if not input.node_tree.nodes['Group Output']:
		print("No group output in node group {}".format(input.name))
		return default
	if not input.node_tree.nodes['Group Input']:
		print("No group input in node group {}".format(input.name))
		return default
	subroot = input.node_tree.nodes['Group Output']
	# Hack - Just blindly find first shader input and use that
	# TODO: Should handle more than one shader output for a node group
	first_output = None
	for sub_output in subroot.inputs:
		if sub_output.is_linked and sub_output.bl_idname in out_sock_types:
			first_output = sub_output
	if not first_output:
		print("Couldn't find shader output in subtree {}".format(input.name))
		return default
	subtree_inputs = {}
	for bl_input in input.inputs:
		match bl_input.bl_idname:
			case 'NodeSocketFloat' | 'NodeSocketFloatFactor':
				subtree_inputs[bl_input.name] = parse_value(bl_input, group_inputs)
			case 'NodeSocketVector':
				subtree_inputs[bl_input.name] = parse_vector(bl_input, group_inputs)
			case 'NodeSocketColor':
				subtree_inputs[bl_input.name] = parse_color(bl_input, group_inputs)
			case 'NodeSocketShader':
				subtree_inputs[bl_input.name] = parse_node(bl_input, group_inputs)
	return sub_parser(first_output.links[0].from_node, subtree_inputs)

# group_inputs is populated when we're parsing a node group (see ShaderNodeGroup below)
def parse_node(input, group_inputs=None):
	match input.bl_idname:
		case 'ShaderNodeGroup':
			return parse_subtree(input, ['NodeSocketShader'], parse_node, group_inputs, warning_shader)
		case 'NodeReroute':
			return parse_node(input.inputs[0], group_inputs)
		case 'NodeSocketShader':
			if input.is_linked:
				if input.links[0].from_node.bl_idname == 'NodeGroupInput':
					return group_inputs[input.links[0].from_socket.name]
				return parse_node(input.links[0].from_node, group_inputs)
			return None
		case 'ShaderNodeBsdfDiffuse':
			color = parse_color(input.inputs['Color'], group_inputs)
			return NodeShaderDiffuse(color) # note: missing roughness + normal
		case 'ShaderNodeBsdfGlass':
			color = parse_color(input.inputs['Color'], group_inputs)
			rough = parse_value(input.inputs['Roughness'], group_inputs)
			ior   = parse_value(input.inputs['IOR'], group_inputs)
			# note: skipping normal
			return NodeShaderGlass(color, rough, ior)
		case 'ShaderNodeBsdfTransparent':
			color = parse_color(input.inputs['Color'], group_inputs)
			return NodeShaderTransparent(color)
		case 'ShaderNodeBsdfTranslucent':
			color = parse_color(input.inputs['Color'], group_inputs)
			return NodeShaderTranslucent(color)
		case 'ShaderNodeBackground':
			color = parse_color(input.inputs['Color'], group_inputs)
			# Blender doesn't specify the pose here
			pose = NodeVectorConstant(cr_vector(0.0, 0.0, 0.0))
			strength = parse_value(input.inputs['Strength'], group_inputs)
			return NodeShaderBackground(color, pose, strength)
		case 'ShaderNodeMixShader':
			factor = parse_value(input.inputs[0], group_inputs)
			a = parse_node(input.inputs[1], group_inputs)
			b = parse_node(input.inputs[2], group_inputs)
			return NodeShaderMix(a, b, factor)
		case 'ShaderNodeBsdfTransparent':
			color = parse_color(input.inputs['Color'], group_inputs)
			return NodeShaderTransparent(color)
		case 'ShaderNodeBsdfTranslucent':
			color = parse_color(input.inputs['Color'], group_inputs)
			return NodeShaderTranslucent(color)
		case 'ShaderNodeEmission':
			color = parse_color(input.inputs['Color'], group_inputs)
			strength = parse_value(input.inputs['Strength'], group_inputs)
			return NodeShaderEmissive(color, strength)
		case 'ShaderNodeAddShader':
			a = parse_node(input.inputs[0], group_inputs)
			b = parse_node(input.inputs[1], group_inputs)
			return NodeShaderAdd(a, b)
		case 'ShaderNodeBsdfPrincipled':
			# I haven't read how this works, so for now, we just patch in a rough approximation
			# Patch behaves slightly differently to a real principled shader, primarily because we don't handle the specular portion. We also don't support subsurface or 'sheen'
			base_color = parse_color(input.inputs['Base Color'], group_inputs)
			base_metallic = parse_value(input.inputs['Metallic'], group_inputs)
			base_roughness = parse_value(input.inputs['Roughness'], group_inputs)
			base_ior = parse_value(input.inputs['IOR'], group_inputs)
			base_alpha = parse_value(input.inputs['Alpha'], group_inputs)

			transmission_weight = parse_value(input.inputs['Transmission Weight'], group_inputs)

			coat_ior = parse_value(input.inputs['Coat IOR'], group_inputs)
			coat_roughness = parse_value(input.inputs['Coat Roughness'], group_inputs)
			coat_tint = parse_color(input.inputs['Coat Tint'], group_inputs)
			coat_weight = parse_value(input.inputs['Coat Weight'], group_inputs)

			emission_color = parse_color(input.inputs['Emission Color'], group_inputs)
			emission_strength = parse_value(input.inputs['Emission Strength'], group_inputs)

			return build_fake_principled(
				base_color,
				base_metallic,
				base_roughness,
				base_ior,
				base_alpha,
				transmission_weight,
				coat_ior,
				coat_roughness,
				coat_tint,
				coat_weight,
				emission_color,
				emission_strength
			)
		case _:
			print("Unknown shader node of type {}, maybe fix.".format(input.bl_idname))
			return warning_shader

def build_fake_principled(base_color, base_metallic, base_roughness, base_ior, base_alpha, transmission_weight, coat_ior, coat_roughness, coat_tint, coat_weight, emission_color, emission_strength):
	base = NodeShaderMix(NodeShaderDiffuse(base_color), NodeShaderTranslucent(base_color), transmission_weight)
	coat = NodeShaderPlastic(base_color, coat_roughness, coat_ior)
	base_and_coat = NodeShaderMix(base, coat, coat_weight)
	metal = NodeShaderMetal(base_color, base_roughness)
	base_and_coat_and_metal = NodeShaderMix(base_and_coat, metal, base_metallic)
	with_alpha = NodeShaderMix(NodeShaderTransparent(NodeColorConstant(cr_color(1.0, 1.0, 1.0, 1.0))), base_and_coat_and_metal, base_alpha)
	emission = NodeShaderEmissive(emission_color, emission_strength)
	with_emission = NodeShaderAdd(emission, with_alpha)
	return with_emission
