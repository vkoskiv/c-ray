import ctypes as ct
from enum import IntEnum
from . node import _vector
from . node import _value
from . node import _color

class _value_arg_fresnel(ct.Structure):
	_fields_ = [
		("IOR", ct.POINTER(_value)),
		("normal", ct.POINTER(_vector))
	]

class _value_arg_map_range(ct.Structure):
	_fields_ = [
		("input_value", ct.POINTER(_value)),
		("from_min", ct.POINTER(_value)),
		("from_max", ct.POINTER(_value)),
		("to_min", ct.POINTER(_value)),
		("to_max", ct.POINTER(_value)),
	]

class _value_arg_alpha(ct.Structure):
	_fields_ = [
		("color", ct.POINTER(_color))
	]

class _component(IntEnum):
	X = 0
	Y = 1
	Z = 3
	U = 4
	V = 5
	F = 6

class _value_arg_vec_to_value(ct.Structure):
	_fields_ = [
		("comp", ct.c_int), # _component
		("vec", ct.POINTER(_vector))
	]

# These are ripped off here:
# https:#docs.blender.org/manual/en/latest/render/shader_nodes/converter/math.html
# TODO: Commented ones need to be implemented to reach parity with Cycles. Feel free to do so! :^)
class _math_op(IntEnum):
	Add = 0
	Subtract = 1
	Multiply = 2
	Divide = 3
	#MultiplyAdd = 
	Power = 4
	Log = 5
	SquareRoot = 6
	InvSquareRoot = 7
	Absolute = 8
	#Exponent = 
	Min = 9
	Max = 10
	LessThan = 11
	GreaterThan = 12
	Sign = 13
	Compare = 14
	#SmoothMin = 
	#SmoothMax = 
	Round = 15
	Floor = 16
	Ceil = 17
	Truncate = 18
	Fraction = 19
	Modulo = 20
	#Wrap = 
	#Snap = 
	#PingPong = 
	Sine = 21
	Cosine = 22
	Tangent = 23
	#ArcSine = 
	#ArcCosine = 
	#ArcTangent = 
	#ArcTan2 = 
	#HyperbolicSine = 
	#HyperbolicCosine = 
	#HyperbolicTangent = 
	ToRadians = 24
	ToDegrees = 25

class _value_arg_math(ct.Structure):
	_fields_ = [
		("A", ct.POINTER(_value)),
		("B", ct.POINTER(_value)),
		("op", ct.c_int) # _math_op
	]

class _value_arg_grayscale(ct.Structure):
	_fields_ = [
		("color", ct.POINTER(_color))
	]

class _value_arg(ct.Union):
	_fields_ = [
		("constant", ct.c_double),
		("fresnel", _value_arg_fresnel),
		("map_range", _value_arg_map_range),
		("alpha", _value_arg_alpha),
		("vec_to_value", _value_arg_vec_to_value),
		("math", _value_arg_math),
		("grayscale", _value_arg_grayscale)
	]

class _value_type(IntEnum):
	unknown      = 0
	constant     = 1
	fresnel      = 2
	map_range    = 3
	raylength    = 4
	alpha        = 5
	vec_to_value = 6
	math         = 7
	grayscale    = 8

_value._anonymous_ = ("arg",)
_value._fields_ = [
		("type", ct.c_int), # _value_type
		("arg", _value_arg)
	]

class NodeValueBase:
	def __init__(self):
		self.cr_struct = _value()
	def castref(self):
		ref = ct.byref(self.cr_struct)
		return ct.cast(ref, ct.POINTER(_value))

class NodeValueConstant(NodeValueBase):
	def __init__(self, constant):
		super().__init__()
		self.constant = constant
		self.cr_struct.type = _value_type.constant
		self.cr_struct.constant = self.constant

class NodeValueFresnel(NodeValueBase):
	def __init__(self, IOR, normal):
		super().__init__()
		self.IOR = IOR
		self.normal = normal
		self.cr_struct.type = _value_type.fresnel
		self.cr_struct.fresnel = _value_arg_fresnel(self.IOR.castref(), self.normal.castref())

class NodeValueMapRange(NodeValueBase):
	def __init__(self, input_value, from_min, from_max, to_min, to_max):
		super().__init__()
		self.input_value = input_value
		self.from_min = from_min
		self.from_max = from_max
		self.to_min = to_min
		self.to_max = to_max
		self.cr_struct.type = _value_type.map_range
		self.cr_struct.map_range = _value_arg_map_range(self.input_value.castref(), self.from_min.castref(), self.from_max.castref(), self.to_min.castref(), self.to_max.castref())

class NodeValueAlpha(NodeValueBase):
	def __init__(self, color):
		super().__init__()
		self.color = color
		self.cr_struct.type = _value_type.alpha
		self.cr_struct.alpha = _value_arg_alpha(self.color.castref())

class NodeValueVecToValue(NodeValueBase):
	def __init__(self, comp, vec):
		super().__init__()
		self.comp = comp
		self.vec = vec
		self.cr_struct.type = _value_type.vec_to_value
		self.cr_struct.vec_to_value = _value_arg_vec_to_value(self.comp, self.vec.castref())

class NodeValueMath(NodeValueBase):
	def __init__(self, a, b, op):
		super().__init__()
		self.a = a
		self.b = b
		self.op = op
		self.cr_struct.type = _value_type.math
		self.cr_struct.math = _value_arg_math(self.a.castref(), self.b.castref(), self.op)

class NodeValueGrayscale(NodeValueBase):
	def __init__(self, color):
		super().__init__()
		self.color = color
		self.cr_struct.type = _value_type.grayscale
		self.cr_struct.grayscale = _value_arg_grayscale(self.color.castref())
