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
		

def convert_node_tree(bl_depsgraph, mat, nt):
	if len(nt.nodes) < 1:
		print("No nodes found for material {}, bailing out".format(mat.name))
		return None
	if not 'Material Output' in nt.nodes:
		print("No Material Output node found in tree for {}, bailing out".format(mat.name))
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

def parse_color(input):
	match input.bl_idname:
		case 'NodeSocketColor':
			if input.is_linked:
				return parse_color(input.links[0].from_node)
			vals = input.default_value
			return NodeColorConstant(cr_color(vals[0], vals[1], vals[2], vals[3]))
		case 'ShaderNodeTexImage':
			if input.image is None:
				print("No image set in blender image texture {}".format(input.name))
				return warning_color
			path = input.image.filepath_from_user()
			return NodeColorImageTexture(path, 0)
		case 'ShaderNodeTexChecker':
			color1 = parse_color(input.inputs['Color1'])
			color2 = parse_color(input.inputs['Color2'])
			scale = parse_value(input.inputs['Scale'])
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
			factor = parse_value(input.inputs[0])
			if input.inputs[4].is_linked and input.inputs[5].is_linked:
				return NodeColorVecToColor(parse_vector(input))

			a = parse_color(input.inputs[6])
			b = parse_color(input.inputs[7])
			return NodeColorMix(a, b, factor)
		# case 'ShaderNodeBlackbody':
		# 	return warning_color
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

def map_math_op(bl_op):
	m = _math_op
	match bl_op:
		case 'ADD':
			return m.Add
		case 'SUBTRACT':
			return m.Subtract
		case 'MULTIPLY':
			return m.Multiply
		case 'DIVIDE':
			return m.Divide
		# case 'MULTIPLY_ADD':
		case 'POWER':
			return m.Power
		case 'LOGARITHM':
			return m.Log
		case 'SQRT':
			return m.SquareRoot
		case 'INVERSE_SQRT':
			return m.InvSquareRoot
		case 'ABSOLUTE':
			return m.Absolute
		# case 'EXPONENT':
		case 'MINIMUM':
			return m.Min
		case 'MAXIMUM':
			return m.Max
		case 'LESS_THAN':
			return m.LessThan
		case 'GREATER_THAN':
			return m.GreaterThan
		case 'SIGN':
			return m.Sign
		case 'COMPARE':
			# FIXME: Ignoring c, using hard-coded epsilon
			return m.Compare
		# case 'SMOOTH_MIN':
		# case 'SMOOTH_MAX':
		case 'ROUND':
			return m.Round
		case 'FLOOR':
			return m.Floor
		case 'CEIL':
			return m.Ceil
		case 'TRUNC':
			return m.Truncate
		case 'FRACT':
			return m.Fraction
		case 'MODULO':
			return m.Modulo
		# case 'FLOORED_MODULO':
		# case 'WRAP':
		# case 'SNAP':
		# case 'PINGPONG':
		case 'SINE':
			return m.Sine
		case 'COSINE':
			return m.Cosine
		case 'TANGENT':
			return m.Tangent
		# case 'ARCSINE':
		# case 'ARCCOSINE':
		# case 'ARCTANGENT':
		# case 'ARCTAN2':
		# case 'SINH':
		# case 'COSH':
		# case 'TANH':
		case 'RADIANS':
			return m.ToRadians
		case 'DEGREES':
			return m.ToDegrees
		case _:
			print("Unknown math op {}, defaulting to ADD".format(bl_op))
			return m.Add

def parse_value(input):
	match input.bl_idname:
		case 'NodeSocketFloat':
			if input.is_linked:
				return parse_value(input.links[0].from_node)
			return NodeValueConstant(input.default_value)
		case 'NodeSocketFloatFactor':
			# note: same as Float, but range is [0,1]
			if input.is_linked:
				return parse_value(input.links[0].from_node)
			return NodeValueConstant(input.default_value)
		case 'ShaderNodeValue':
			value = input.outputs[0].default_value
			return NodeValueConstant(value)
		case 'ShaderNodeMath':
			a = parse_value(input.inputs[0])
			b = parse_value(input.inputs[1])
			# c = parse_value(input.inputs[2])
			op = map_math_op(input.operation)
			return NodeValueMath(a, b, op)
		case 'ShaderNodeVectorMath':
			vec = parse_vector(input)
			return NodeValueVecToValue(_component.F, vec)
		case _:
			print("Unknown value node of type {}, maybe fix.".format(input.bl_idname))

# From here: https://docs.blender.org/api/current/bpy_types_enum_items/node_vec_math_items.html#rna-enum-node-vec-math-items
# Commented ones aren't implemented yet, and will emit a notice to stdout
def map_vec_op(bl_op):
	v = _vector_op
	match bl_op:
		case 'ADD':
			return v.Add
		case 'SUBTRACT':
			return v.Subtract
		case 'MULTIPLY':
			return v.Multiply
		case 'DIVIDE':
			return v.Divide
		# case 'MULTIPLY_ADD':
		case 'CROSS_PRODUCT':
			return v.Cross
		# case 'PROJECT':
		case 'REFLECT':
			return v.Reflect
		case 'REFRACT':
			return v.Refract
		# case 'FACEFORWARD':
		case 'DOT_PRODUCT':
			return v.Dot
		case 'DISTANCE':
			return v.Distance
		case 'LENGTH':
			return v.Length
		case 'SCALE':
			return v.Scale
		case 'NORMALIZE':
			return v.Normalize
		case 'ABSOLUTE':
			return v.Abs
		case 'MINIMUM':
			return v.Min
		case 'MAXIMUM':
			return v.Max
		case 'FLOOR':
			return v.Floor
		case 'CEIL':
			return v.Ceil
		# case 'FRACTION':
		case 'MODULO':
			return v.Modulo
		case 'WRAP':
			return v.Wrap
		# case 'SNAP':
		case 'SINE':
			return v.Sin
		case 'COSINE':
			return v.Cos
		case 'TANGENT':
			return v.Tan
		case _:
			print("Unknown vector op {}, defaulting to ADD".format(bl_op))
			return v.Add

def parse_vector(input):
	match input.bl_idname:
		case 'NodeSocketVector':
			if input.is_linked:
				return parse_vector(input.links[0].from_node)
			vec = input.default_value
			return NodeVectorConstant(cr_vector(vec[0], vec[1], vec[2]))
		case 'ShaderNodeMix':
			a = parse_vector(input.inputs[4])
			b = parse_vector(input.inputs[5])
			factor = parse_value(input.inputs[0])
			return NodeVectorVecMix(a, b, factor)
		case 'ShaderNodeVectorMath':
			a = parse_vector(input.inputs[0])
			b = parse_vector(input.inputs[1])
			c = parse_vector(input.inputs[2])
			f = parse_value(input.inputs[3])
			op = map_vec_op(input.operation)
			return NodeVectorVecMath(a, b, c, f, op)
		case 'ShaderNodeTexCoord':
			if input.outputs['UV'].is_linked:
				return NodeVectorUV()
			if input.outputs['Normal'].is_linked:
				return NodeVectorNormal()
			print("Unsupported ShaderNodeTexCoord, here's a dump:")
			for output in input.outputs:
				print("{}: {}".format(output.name, output.is_linked))
		case _:
			print("Unknown vector node of type {}, maybe fix.".format(input.bl_idname))

warning_shader = NodeShaderDiffuse(warning_color)

def parse_node(input):
	match input.bl_idname:
		case 'NodeSocketShader':
			if input.is_linked:
				return parse_node(input.links[0].from_node)
			return None
		case 'ShaderNodeBsdfDiffuse':
			color = parse_color(input.inputs['Color'])
			return NodeShaderDiffuse(color) # note: missing roughness + normal
		case 'ShaderNodeBsdfGlass':
			color = parse_color(input.inputs['Color'])
			rough = parse_value(input.inputs['Roughness'])
			ior   = parse_value(input.inputs['IOR'])
			# note: skipping normal
			return NodeShaderGlass(color, rough, ior)
		case 'ShaderNodeBsdfTransparent':
			color = parse_color(input.inputs['Color'])
			return NodeShaderTransparent(color)
		case 'ShaderNodeBsdfTranslucent':
			color = parse_color(input.inputs['Color'])
			return NodeShaderTranslucent(color)
		case 'ShaderNodeBackground':
			color = parse_color(input.inputs['Color'])
			# Blender doesn't specify the pose here
			pose = NodeVectorConstant(cr_vector(0.0, 0.0, 0.0))
			strength = parse_value(input.inputs['Strength'])
			return NodeShaderBackground(color, pose, strength)
		case 'ShaderNodeMixShader':
			factor = parse_value(input.inputs[0])
			a = parse_node(input.inputs[1])
			b = parse_node(input.inputs[2])
			return NodeShaderMix(a, b, factor)
		case 'ShaderNodeBsdfTransparent':
			color = parse_color(input.inputs['Color'])
			return NodeShaderTransparent(color)
		case 'ShaderNodeBsdfTranslucent':
			color = parse_color(input.inputs['Color'])
			return NodeShaderTranslucent(color)
		case 'ShaderNodeEmission':
			color = parse_color(input.inputs['Color'])
			strength = parse_value(input.inputs['Strength'])
			return NodeShaderEmissive(color, strength)
		case 'ShaderNodeAddShader':
			a = parse_node(input.inputs[0])
			b = parse_node(input.inputs[1])
			return NodeShaderAdd(a, b)
		case 'ShaderNodeBsdfPrincipled':
			# I haven't read how this works, so for now, we just patch in a rough approximation
			# Patch behaves slightly differently to a real principled shader, primarily because we don't handle the specular portion. We also don't support subsurface or 'sheen'
			base_color = parse_color(input.inputs['Base Color'])
			base_metallic = parse_value(input.inputs['Metallic'])
			base_roughness = parse_value(input.inputs['Roughness'])
			base_ior = parse_value(input.inputs['IOR'])
			base_alpha = parse_value(input.inputs['Alpha'])

			transmission_weight = parse_value(input.inputs['Transmission Weight'])

			coat_ior = parse_value(input.inputs['Coat IOR'])
			coat_roughness = parse_value(input.inputs['Coat Roughness'])
			coat_tint = parse_color(input.inputs['Coat Tint'])
			coat_weight = parse_value(input.inputs['Coat Weight'])

			emission_color = parse_color(input.inputs['Emission Color'])
			emission_strength = parse_value(input.inputs['Emission Strength'])

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
