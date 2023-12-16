import ctypes as ct
from enum import IntEnum
from . node import _vector
from . node import _value
from . node import _color

class cr_vector(ct.Structure):
	_fields_ = [
		("x", ct.c_float),
		("y", ct.c_float),
		("z", ct.c_float),
	]

class cr_coord(ct.Structure):
	_fields_ = [
		("u", ct.c_float),
		("v", ct.c_float)
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
		("A", ct.POINTER(_vector)),
		("B", ct.POINTER(_vector)),
		("C", ct.POINTER(_vector)),
		("f", ct.POINTER(_value)),
		("op", ct.c_int) # _vector_op
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

class _vector_type(IntEnum):
	unknown  = 0
	constant = 1
	normal   = 2
	uv       = 3
	vecmath  = 4
	mix      = 5

_vector._anonymous_ = ("arg",)
_vector._fields_ = [
		("type", ct.c_int), # _vector_type
		("arg", _vector_arg)
	]

class NodeVectorBase:
	def __init__(self):
		self.cr_struct = _vector()
	def castref(self):
		ref = ct.byref(self.cr_struct)
		return ct.cast(ref, ct.POINTER(_vector))

class NodeVectorConstant(NodeVectorBase):
	def __init__(self, vector):
		super().__init__()
		self.vector = vector
		self.cr_struct.type = _vector_type.constant
		self.cr_struct.constant = self.vector

class NodeVectorNormal(NodeVectorBase):
	def __init__(self):
		super().__init__()
		self.cr_struct.type = _vector_type.normal

class NodeVectorUV(NodeVectorBase):
	def __init__(self):
		super().__init__()
		self.cr_struct.type = _vector_type.uv

class NodeVectorVecMath(NodeVectorBase):
	def __init__(self, a, b, c, f, op):
		super().__init__()
		self.a = a
		self.b = b
		self.c = c
		self.f = f
		self.op = op
		self.cr_struct.type = _vector_type.vecmath
		self.cr_struct.vecmath = _vector_arg_vecmath(self.a.castref(), self.b.castref(), self.c.castref(), self.f.castref(), self.op)

class NodeVectorVecMix(NodeVectorBase):
	def __init__(self, a, b, factor):
		super().__init__()
		self.a = a
		self.b = b
		self.factor = factor
		self.cr_struct.type = _vector_type.mix
		self.cr_struct.vec_mix = _vector_arg_vec_mix(self.a.castref(), self.b.castref(), self.factor.castref())
