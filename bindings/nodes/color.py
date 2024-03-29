import ctypes as ct
from enum import IntEnum
from . node import _vector
from . node import _value
from . node import _color

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

class _color_arg_hsv(ct.Structure):
	_fields_ = [
		("H", ct.POINTER(_value)),
		("S", ct.POINTER(_value)),
		("V", ct.POINTER(_value)),
	]

class _color_arg_hsv_tform(ct.Structure):
	_fields_ = [
		("tex", ct.POINTER(_color)),
		("H", ct.POINTER(_value)),
		("S", ct.POINTER(_value)),
		("V", ct.POINTER(_value)),
		("f", ct.POINTER(_value)),
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

class _color_arg_color_mix(ct.Structure):
	_fields_ = [
		("a", ct.POINTER(_color)),
		("b", ct.POINTER(_color)),
		("factor", ct.POINTER(_value))
	]

class ramp_element(ct.Structure):
	_fields_ = [
		("color", cr_color),
		("position", ct.c_float)
	]

class color_mode(IntEnum):
	mode_rgb = 0
	mode_hsv = 1
	mode_hsl = 2

class interpolation(IntEnum):
	ease = 0
	cardinal = 1
	linear = 2
	b_spline = 3
	constant = 4
	
class _color_arg_color_ramp(ct.Structure):
	_fields_ = [
		("factor", ct.POINTER(_value)),
		("color_mode", ct.c_int),
		("interpolation", ct.c_int),
		("elements", ct.POINTER(ramp_element)),
		("element_count", ct.c_int)
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
		("hsv", _color_arg_hsv),
		("hsv_tform", _color_arg_hsv_tform),
		("vec_to_color", _color_arg_vec_to_color),
		("gradient", _color_arg_gradient),
		("color_mix", _color_arg_color_mix),
		("color_ramp", _color_arg_color_ramp),
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
	hsv          = 8
	hsv_tform    = 9
	vec_to_color = 10
	gradient     = 11
	color_mix    = 12
	color_ramp   = 13

_color._anonymous_ = ("arg",)
_color._fields_ = [
		("type", ct.c_int), # _color_type
		("arg", _color_arg),
	]

class NodeColorBase:
	def __init__(self):
		self.cr_struct = _color()
	def castref(self):
		ref = ct.byref(self.cr_struct)
		return ct.cast(ref, ct.POINTER(_color))

class NodeColorConstant(NodeColorBase):
	def __init__(self, color):
		super().__init__()
		self.color = color
		self.cr_struct.type = _color_type.constant
		self.cr_struct.constant = self.color

class NodeColorImageTexture(NodeColorBase):
	def __init__(self, full_path, options):
		super().__init__()
		self.full_path = full_path
		self.options = options
		self.cr_struct.type = _color_type.image
		self.cr_struct.image = _color_arg_image(self.full_path.encode(), self.options)

class NodeColorCheckerboard(NodeColorBase):
	def __init__(self, a, b, scale):
		super().__init__()
		self.a = a
		self.b = b
		self.scale = scale
		self.cr_struct.type = _color_type.checkerboard
		self.cr_struct.checkerboard = _color_arg_checkerboard(self.a.castref(), self.b.castref(), self.scale.castref())

class NodeColorBlackbody(NodeColorBase):
	def __init__(self, degrees):
		super().__init__()
		self.degrees = degrees
		self.cr_struct.type = _color_type.blackbody
		self.cr_struct.blackbody = _color_arg_blackbody(self.degrees.castref())

class NodeColorSplit(NodeColorBase):
	def __init__(self, node):
		super().__init__()
		self.node = node
		self.cr_struct.type = _color_type.split
		self.cr_struct.split = _color_arg_split(self.node.castref())

class NodeColorRGB(NodeColorBase):
	def __init__(self, r, g, b):
		super().__init__()
		self.r = r
		self.g = g
		self.b = b
		self.cr_struct.type = _color_type.rgb
		self.cr_struct.rgb = _color_arg_rgb(self.r.castref(), self.g.castref(), self.b.castref())
		
class NodeColorHSL(NodeColorBase):
	def __init__(self, h, s, l):
		super().__init__()
		self.h = h
		self.s = s
		self.l = l
		self.cr_struct.type = _color_type.hsl
		self.cr_struct.hsl = _color_arg_hsl(self.h.castref(), self.s.castref(), self.l.castref())

class NodeColorHSV(NodeColorBase):
	def __init__(self, h, s, v):
		super().__init__()
		self.h = h
		self.s = s
		self.v = v
		self.cr_struct.type = _color_type.hsv
		self.cr_struct.hsv = _color_arg_hsv(self.h.castref(), self.s.castref(), self.v.castref())

class NodeColorHSVTransform(NodeColorBase):
	def __init__(self, tex, h, s, v, f):
		super().__init__()
		self.tex = tex
		self.h = h
		self.s = s
		self.v = v
		self.f = f
		self.cr_struct.type = _color_type.hsv_tform
		self.cr_struct.hsv_tform = _color_arg_hsv_tform(self.tex.castref(), self.h.castref(), self.s.castref(), self.v.castref(), self.f.castref())

class NodeColorVecToColor(NodeColorBase):
	def __init__(self, vec):
		super().__init__()
		self.vec = vec
		self.cr_struct.type = _color_type.vec_to_color
		self.cr_struct.vec_to_color = _color_arg_vec_to_color(self.vec.castref())

class NodeColorGradient(NodeColorBase):
	def __init__(self, a, b):
		super().__init__()
		self.a = a
		self.b = b
		self.cr_struct.type = _color_type.gradient
		self.cr_struct.gradient = _color_arg_gradient(self.a.castref(), self.b.castref())
		
class NodeColorMix(NodeColorBase):
	def __init__(self, a, b, factor):
		super().__init__()
		self.a = a
		self.b = b
		self.factor = factor
		self.cr_struct.type = _color_type.color_mix
		self.cr_struct.color_mix = _color_arg_color_mix(self.a.castref(), self.b.castref(), self.factor.castref())

class NodeColorRamp(NodeColorBase):
	def __init__(self, factor, color_mode, interpolation, elements):
		super().__init__()
		self.factor = factor
		self.color_mode = color_mode
		self.interpolation = interpolation
		self.elements = (ramp_element * len(elements))(*elements)
		self.element_count = len(elements)
		self.cr_struct.color_ramp = _color_arg_color_ramp(self.factor.castref(), self.color_mode, self.interpolation, self.elements, self.element_count)
