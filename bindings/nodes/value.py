import ctypes as ct
from vector import _vector
from color import _color

class _value_type(IntEnum):
	unknown      = 1
	constant     = 2
	fresnel      = 3
	map_range    = 4
	raylength    = 5
	alpha        = 6
	vec_to_value = 7
	math         = 8
	grayscale    = 9

class _value(ct.Structure):
	_fields_ = [
		("type", _value_type)
		("arg", _value_arg)
	]

class _value_arg(ct.Union):
	_fields_ = [
		("constant", ct.c_double),
		("fresnel", _value_arg_fresnel),
		("map_range", _value_arg_map_range)
	]

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
		("comp", _component),
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
		("op", _math_op)
	]

class _value_arg_grayscale(ct.Structure):
	_fields_ = [
		("color", ct.POINTER(_color))
	]
