# Convert Cycles nodegraphs to c-ray nodegraphs

from . shader import *
from . color import *
from . value import *
from . vector import *

def convert_background(nt):
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
	return parse_node(root.inputs['Surface'].links[0].from_node)

warning_color = NodeColorCheckerboard(NodeColorConstant(cr_color(1.0, 0.0, 0.0, 0.0)), NodeColorConstant(cr_color(1.0, 1.0, 1.0, 1.0)), NodeValueConstant(100.0))

def parse_color(input):
	match input.bl_idname:
		case 'NodeSocketColor':
			if input.is_linked:
				return parse_color(input.links[0].from_node)
			vals = input.default_value
			return NodeColorConstant(cr_color(vals[0], vals[1], vals[2], vals[3]))
		# case 'ShaderNodeTexImage':
		# 	return warning_color
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
			a = parse_color(input.inputs[6])
			b = parse_color(input.inputs[7])
			factor = parse_value(input.inputs[0])
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
		case _:
			print("Unknown value node of type {}, maybe fix.".format(input.bl_idname))

def parse_vector(input):
	match input.bl_idname:
		case 'NodeSocketVector':
			if input.is_linked:
				return parse_vector(input.links[0].from_node)
			vec = input.default_value
			return NodeVectorConstant(cr_vector(vec[0], vec[1], vec[2]))
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
