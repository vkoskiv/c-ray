import ctypes as ct
from enum import IntEnum
from . node import _vector
from . node import _value
from . node import _color

# class _color(ct.Structure):
# 	pass

class cr_color(ct.Structure):
	_fields_ = [
		("r", ct.c_float),
		("g", ct.c_float),
		("b", ct.c_float),
		("a", ct.c_float),
	]

class _color_arg_image(ct.Structure):
	_fields_ = [
		("full_path", ct.c_char_p),
		("options", ct.c_uint8)
	]

class _color_arg_checkerboard(ct.Structure):
	_fields_ = [
		("a", ct.POINTER(_color)),
		("b", ct.POINTER(_color)),
		("scale", ct.POINTER(_value))
	]

class _color_arg_blackbody(ct.Structure):
	_fields_ = [
		("degrees", ct.POINTER(_value))
	]

class _color_arg_split(ct.Structure):
	_fields_ = [
		("value", ct.POINTER(_value))
	]

class _color_arg_rgb(ct.Structure):
	_fields_ = [
		("red", ct.POINTER(_value)),
		("green", ct.POINTER(_value)),
		("blue", ct.POINTER(_value)),
	]

class _color_arg_hsl(ct.Structure):
	_fields_ = [
		("H", ct.POINTER(_value)),
		("S", ct.POINTER(_value)),
		("L", ct.POINTER(_value)),
	]

class _color_arg_vec_to_color(ct.Structure):
	_fields_ = [
		("vec", ct.POINTER(_vector))
	]

class _color_arg_gradient(ct.Structure):
	_fields_ = [
		("a", ct.POINTER(_color)),
		("b", ct.POINTER(_color)),
	]

class _color_arg(ct.Union):
	_fields_ = [
		("constant", cr_color),
		("image", _color_arg_image),
		("checkerboard", _color_arg_checkerboard),
		("blackbody", _color_arg_blackbody),
		("split", _color_arg_split),
		("rgb", _color_arg_rgb),
		("hsl", _color_arg_hsl),
		("vec_to_color", _color_arg_vec_to_color),
		("gradient", _color_arg_gradient),
	]

class _color_type(IntEnum):
	unknown      = 0
	constant     = 1
	image        = 2
	checkerboard = 3
	blackbody    = 4
	split        = 5
	rgb          = 6
	hsl          = 7
	vec_to_color = 8
	gradient     = 9

_color._anonymous_ = ("arg",)
_color._fields_ = [
		("type", ct.c_int), # _color_type
		("arg", _color_arg),
	]

class ColorConstant:
	def __init__(self, color):
		self.color = color
		self.cr_struct = _color()
		self.cr_struct.type = _color_type.constant
		self.cr_struct.constant = self.color
