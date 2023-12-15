import ctypes as ct
from enum import IntEnum
from . node import _color
from . node import _value
from . node import _vector

class _shader(ct.Structure):
	pass

class _shader_arg_diffuse(ct.Structure):
	_fields_ = [
		("color", ct.POINTER(_color))
	]

class _shader_arg_metal(ct.Structure):
	_fields_ = [
		("color", ct.POINTER(_color)),
		("roughness", ct.POINTER(_value)),
	]

class _shader_arg_glass(ct.Structure):
	_fields_ = [
		("color", ct.POINTER(_color)),
		("roughness", ct.POINTER(_value)),
		("IOR", ct.POINTER(_value)),
	]

class _shader_arg_plastic(ct.Structure):
	_fields_ = [
		("color", ct.POINTER(_color)),
		("roughness", ct.POINTER(_value)),
		("IOR", ct.POINTER(_value)),
	]

class _shader_arg_mix(ct.Structure):
	_fields_ = [
		("A", ct.POINTER(_shader)),
		("B", ct.POINTER(_shader)),
		("factor", ct.POINTER(_value))
	]

class _shader_arg_add(ct.Structure):
	_fields_ = [
		("A", ct.POINTER(_shader)),
		("B", ct.POINTER(_shader)),
	]

class _shader_arg_transparent(ct.Structure):
	_fields_ = [
		("color", ct.POINTER(_color))
	]

class _shader_arg_emissive(ct.Structure):
	_fields_ = [
		("color", ct.POINTER(_color)),
		("strength", ct.POINTER(_value))
	]

class _shader_arg_translucent(ct.Structure):
	_fields_ = [
		("color", ct.POINTER(_color))
	]

class _shader_arg_background(ct.Structure):
	_fields_ = [
		("color", ct.POINTER(_color)),
		("pose", ct.POINTER(_vector)),
		("strength", ct.POINTER(_value))
	]

class _shader_type(IntEnum):
	unknown     = 0
	diffuse     = 1
	metal       = 2
	glass       = 3
	plastic     = 4
	mix         = 5
	add         = 6
	transparent = 7
	emissive    = 8
	translucent = 9
	background  = 10

class _shader_arg(ct.Union):
	_fields_ = [
		("diffuse", _shader_arg_diffuse),
		("metal", _shader_arg_metal),
		("glass", _shader_arg_glass),
		("plastic", _shader_arg_plastic),
		("mix", _shader_arg_mix),
		("add", _shader_arg_add),
		("transparent", _shader_arg_transparent),
		("emissive", _shader_arg_emissive),
		("translucent", _shader_arg_translucent),
		("background", _shader_arg_background),
	]

_shader._anonymous_ = ("arg",)
_shader._fields_ = [
		("type", ct.c_int), # _shader_type
		("arg", _shader_arg)
	]

class NodeShaderBase:
	cr_struct = _shader()
	def castref(self):
		ref = ct.byref(self.cr_struct)
		return ct.cast(ref, ct.POINTER(_shader))

class NodeShaderDiffuse(NodeShaderBase):
	def __init__(self, color):
		self.color = color
		self.cr_struct.type = _shader_type.diffuse
		self.cr_struct.diffuse = _shader_arg_diffuse(self.color.castref())

class NodeShaderMetal(NodeShaderBase):
	def __init__(self, color, roughness):
		self.color = color
		self.roughness = roughness
		self.cr_struct.type = _shader_type.metal
		self.cr_struct.metal = _shader_arg_metal(self.color.castref(), self.roughness.castref())

class NodeShaderGlass(NodeShaderBase):
	def __init__(self, color, roughness, IOR):
		self.color = color
		self.roughness = roughness
		self.IOR = IOR
		self.cr_struct.type = _shader_type.glass
		self.cr_struct.glass = _shader_arg_glass(self.color.castref(), self.roughness.castref(), self.IOR.castref())

class NodeShaderPlastic(NodeShaderBase):
	def __init__(self, color, roughness, IOR):
		self.color = color
		self.roughness = roughness
		self.IOR = IOR
		self.cr_struct.type = _shader_type.plastic
		self.cr_struct.plastic = _shader_arg_plastic(self.color.castref(), self.roughness.castref(), self.IOR.castref())

class NodeShaderMix(NodeShaderBase):
	def __init__(self, a, b, factor):
		self.a = a
		self.b = b
		self.factor = factor
		self.cr_struct.type = _shader_type.mix
		self.cr_struct.mix = _shader_arg_mix(self.a.castref(), self.b.castref(), self.factor.castref())

class NodeShaderAdd(NodeShaderBase):
	def __init__(self, a, b):
		self.a = a
		self.b = b
		self.cr_struct.type = _shader_type.add
		self.cr_struct.add = _shader_arg_add(self.a.castref(), self.b.castref())

class NodeShaderTransparent(NodeShaderBase):
	def __init__(self, color):
		self.color = color
		self.cr_struct.type = _shader_type.transparent
		self.cr_struct.transparent = _shader_arg_transparent(self.color.castref())

class NodeShaderEmissive(NodeShaderBase):
	def __init__(self, color, strength):
		self.color = color
		self.strength = strength
		self.cr_struct.type = _shader_type.emissive
		self.cr_struct.emissive = _shader_arg_emissive(self.color.castref(), self.strength.castref())

class NodeShaderTranslucent(NodeShaderBase):
	def __init__(self, color):
		self.color = color
		self.cr_struct.type = _shader_type.translucent
		self.cr_struct.translucent = _shader_arg_translucent(self.color.castref())

class NodeShaderBackground(NodeShaderBase):
	def __init__(self, color, pose, strength):
		self.color = color
		self.pose = pose
		self.strength = strength
		self.cr_struct.type = _shader_type.background
		self.cr_struct.background = _shader_arg_background(self.color.castref(), self.pose.castref(), self.strength.castref())
