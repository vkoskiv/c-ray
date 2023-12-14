import ctypes as ct
from enum import IntEnum
from . node import _value

class _vector_type(IntEnum):
	unknown  = 0
	constant = 1
	normal   = 2
	uv       = 3
	vecmath  = 4
	mix      = 5

class _vector(ct.Structure):
	pass

class cr_vector(ct.Structure):
	_fields_ = [
		("x", ct.c_float),
		("y", ct.c_float),
		("z", ct.c_float),
	]

# These are ripped off here:
# https://docs.blender.org/manual/en/latest/render/shader_nodes/converter/vector_math.html
# TODO: Commented ones need to be implemented to reach parity with Cycles. Feel free to do so! :^)
class _vector_op(IntEnum):
	Add = 0
	Subtract = 1
	Multiply = 2
	Divide = 3
	#MultiplyAdd = 
	Cross = 4
	#Project = 
	Reflect = 5 
	Refract = 6
	#Faceforward = 
	Dot = 7
	Distance = 8
	Length = 9
	Scale = 10
	Normalize = 11
	Wrap = 12
	#Snap = 
	Floor = 13
	Ceil = 14
	Modulo = 15
	#Fraction = 
	Abs = 16
	Min = 17
	Max = 18
	Sin = 19
	Cos = 20
	Tan = 21

class _vector_arg_vecmath(ct.Structure):
	_fields_ = [
		("A", ct.POINTER(_vector))
		("B", ct.POINTER(_vector))
		("C", ct.POINTER(_vector))
		("f", ct.POINTER(_value)),
		("op", _vector_op)
	]

class _vector_arg_vec_mix(ct.Structure):
	_fields_ = [
		("A", ct.POINTER(_vector)),
		("B", ct.POINTER(_vector)),
		("factor", ct.POINTER(_value))
	]

class _vector_arg(ct.Union):
	_fields_ = [
		("constant", cr_vector),
		("vecmath", _vector_arg_vecmath),
		("vec_mix", _vector_arg_vec_mix)
	]

_vector._fields_ = [
		("type", _vector_type)
		("arg", _vector_arg)
	]

