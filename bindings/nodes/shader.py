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

class Diffuse:
	def __init__(self, color):
		self.color = color
		self.cr_struct = _shader()
		self.cr_struct.type = _shader_type.diffuse
		ref = ct.byref(self.color.cr_struct)
		casted = ct.cast(ref, ct.POINTER(_color))
		self.cr_struct.diffuse = _shader_arg_diffuse(casted)

