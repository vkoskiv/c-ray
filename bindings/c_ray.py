import ctypes as ct
from contextlib import contextmanager
from enum import IntEnum

from . import cray_wrap as _lib
from . nodes.vector import (
	cr_vector,
	cr_coord
)

class cr_renderer(ct.Structure):
	pass

class cr_bm_union(ct.Union):
	_fields_ = [
		("byte_ptr", ct.POINTER(ct.c_ubyte)),
		("float_ptr", ct.POINTER(ct.c_float))
	]

class cr_bitmap(ct.Structure):
	_fields_ = [
		("colorspace", ct.c_int),
		("precision", ct.c_int),
		("data", cr_bm_union),
		("stride", ct.c_size_t),
		("width", ct.c_size_t),
		("height", ct.c_size_t),
	]

class _cr_rparam(IntEnum):
	# int
	threads = 0
	samples = 1
	bounces = 2
	tile_width = 3
	tile_height = 4
	tile_order = 5
	output_num = 6
	override_width = 7
	override_height = 8
	override_cam = 9
	is_iterative = 10
	# str
	output_path = 11
	asset_path = 12
	output_name = 13
	output_filetype = 14
	node_list = 15
	blender_mode = 16

def _r_set_num(ptr, param, value):
	return _lib.renderer_set_num_pref(ptr, param, value)

def _r_get_num(ptr, param):
	return _lib.renderer_get_num_pref(ptr, param)

def _r_set_str(ptr, param, value):
	return _lib.renderer_set_str_pref(ptr, param, value)

def _r_get_str(ptr, param):
	return _lib.renderer_get_str_pref(ptr, param)

class _pref:
	def __init__(self, r_ptr):
		self.r_ptr = r_ptr

	def _get_threads(self):
		return _r_get_num(self.r_ptr, _cr_rparam.threads)
	def _set_threads(self, value):
		_r_set_num(self.r_ptr, _cr_rparam.threads, value)
	threads = property(_get_threads, _set_threads, None, "Local thread count, defaults to nproc + 2")

	def _get_samples(self):
		return _r_get_num(self.r_ptr, _cr_rparam.samples)
	def _set_samples(self, value):
		_r_set_num(self.r_ptr, _cr_rparam.samples, value)
	samples = property(_get_samples, _set_samples, None, "Amount of samples to render")

	def _get_bounces(self):
		return _r_get_num(self.r_ptr, _cr_rparam.bounces)
	def _set_bounces(self, value):
		_r_set_num(self.r_ptr, _cr_rparam.bounces, value)
	bounces = property(_get_bounces, _set_bounces, None, "Max times a light ray can bounce in the scene")

	def _get_tile_x(self):
		return _r_get_num(self.r_ptr, _cr_rparam.tile_width)
	def _set_tile_x(self, value):
		_r_set_num(self.r_ptr, _cr_rparam.tile_width, value)
	tile_x = property(_get_tile_x, _set_tile_x, None, "Tile width")

	def _get_tile_y(self):
		return _r_get_num(self.r_ptr, _cr_rparam.tile_height)
	def _set_tile_y(self, value):
		_r_set_num(self.r_ptr, _cr_rparam.tile_height, value)
	tile_y = property(_get_tile_y, _set_tile_y, None, "Tile height")

	def _get_tile_order(self):
		return _r_get_num(self.r_ptr, _cr_rparam.tile_order)
	def _set_tile_order(self, value):
		_r_set_num(self.r_ptr, _cr_rparam.tile_order, value)
	tile_order = property(_get_tile_order, _set_tile_order, None, "Order to render tiles in")

	def _get_output_idx(self):
		return _r_get_num(self.r_ptr, _cr_rparam.output_num)
	def _set_output_idx(self, value):
		_r_set_num(self.r_ptr, _cr_rparam.output_num, value)
	output_idx = property(_get_output_idx, _set_output_idx, None, "Number for output file name")

	def _get_img_width(self):
		return _r_get_num(self.r_ptr, _cr_rparam.override_width)
	def _set_img_width(self, value):
		_r_set_num(self.r_ptr, _cr_rparam.override_width, value)
	img_width = property(_get_img_width, _set_img_width, None, "Image width in pixels")

	def _get_img_height(self):
		return _r_get_num(self.r_ptr, _cr_rparam.override_height)
	def _set_img_height(self, value):
		_r_set_num(self.r_ptr, _cr_rparam.override_height, value)
	img_height = property(_get_img_height, _set_img_height, None, "Image height in pixels")

	def _get_should_save(self):
		return _r_get_num(self.r_ptr, _cr_rparam.should_save)
	def _set_should_save(self, value):
		_r_set_num(self.r_ptr, _cr_rparam.should_save, value)
	should_save = property(_get_should_save, _set_should_save, None, "0 = don't save, 1 = save")

	def _get_cam_idx(self):
		return _r_get_num(self.r_ptr, _cr_rparam.override_cam)
	def _set_cam_idx(self, value):
		_r_set_num(self.r_ptr, _cr_rparam.override_cam, value)
	cam_idx = property(_get_cam_idx, _set_cam_idx, None, "Select camera")

	def _get_is_iterative(self):
		return _r_get_num(self.r_ptr, _cr_rparam.is_iterative)
	def _set_is_iterative(self, value):
		_r_set_num(self.r_ptr, _cr_rparam.is_iterative, value)
	is_iterative = property(_get_is_iterative, _set_is_iterative, None, "")

	def _get_output_path(self):
		return _r_get_str(self.r_ptr, _cr_rparam.output_path)
	def _set_output_path(self, value):
		_r_set_str(self.r_ptr, _cr_rparam.output_path, value)
	output_path = property(_get_output_path, _set_output_path, None, "")

	def _get_asset_path(self):
		return _r_get_str(self.r_ptr, _cr_rparam.asset_path)
	def _set_asset_path(self, value):
		_r_set_str(self.r_ptr, _cr_rparam.asset_path, value)
	asset_path = property(_get_asset_path, _set_asset_path, None, "")

	def _get_output_name(self):
		return _r_get_str(self.r_ptr, _cr_rparam.output_name)
	def _set_output_name(self, value):
		_r_set_str(self.r_ptr, _cr_rparam.output_name, value)
	output_name = property(_get_output_name, _set_output_name, None, "")

	def _get_output_filetype(self):
		return _r_get_str(self.r_ptr, _cr_rparam.output_filetype)
	def _set_output_filetype(self, value):
		_r_set_str(self.r_ptr, _cr_rparam.output_filetype, value)
	output_filetype = property(_get_output_filetype, _set_output_filetype, None, "")

	def _get_node_list(self):
		return _r_get_str(self.r_ptr, _cr_rparam.node_list)
	def _set_node_list(self, value):
		_r_set_str(self.r_ptr, _cr_rparam.node_list, value)
	node_list = property(_get_node_list, _set_node_list, None, "")

	def _get_blender_mode(self):
		return _r_get_num(self.r_ptr, _cr_rparam.blender_mode)
	def _set_blender_mode(self, value):
		_r_set_num(self.r_ptr, _cr_rparam.blender_mode, value)
	blender_mode = property(_get_blender_mode, _set_blender_mode, None, "")

class _version:
	def _get_semantic(self):
		return _lib.get_version()
	semantic = property(_get_semantic, None, None, "")
	def _get_hash(self):
		return _lib.get_git_hash()
	githash = property(_get_hash, None, None, "")

version = _version()

threeint = ct.c_int * 3

class cr_face(ct.Structure):
	_fields_ = [
		("vertex_idx", threeint),
		("normal_idx", threeint),
		("texture_idx", threeint),
		("mat_idx", ct.c_uint, 16),
		("has_normals", ct.c_bool, 1)
	]

class mesh:
	def __init__(self, scene_ptr, name):
		self.scene_ptr = scene_ptr
		self.name = name
		self.cr_idx = _lib.scene_mesh_new(self.scene_ptr, self.name)

	def bind_vertex_buf(self, buf):
		_lib.mesh_bind_vertex_buf(self.scene_ptr, self.cr_idx, buf.cr_idx)
	def bind_faces(self, faces, face_count):
		_lib.mesh_bind_faces(self.scene_ptr, self.cr_idx, faces, face_count)

class sphere:
	def __init__(self, scene_ptr, radius):
		self.scene_ptr = scene_ptr
		self.radius = radius
		self.cr_idx = _lib.scene_add_sphere(self.scene_ptr, self.radius)

class cam_param(IntEnum):
	fov = 0
	focus_distance = 1
	fstops = 2
	pose_x = 3
	pose_y = 4
	pose_z = 5
	pose_roll = 6
	pose_pitch = 7
	pose_yaw = 8
	time = 9
	res_x = 10
	res_y = 11
	blender_coord = 12

class camera:
	def __init__(self, scene_ptr):
		self.scene_ptr = scene_ptr
		self.cr_idx = _lib.camera_new(self.scene_ptr)

	def set_param(self, param, value):
		ret =  _lib.camera_set_num_pref(self.scene_ptr, self.cr_idx, param, value)
		# Weird. Could just do this internally, no?
		_lib.camera_update(self.scene_ptr, self.cr_idx)

def inst_type(IntEnum):
	mesh = 0
	sphere = 1

class cr_matrix(ct.Structure):
	_fields_ = [
		("mtx", (ct.c_float * 16))
	]

class instance:
	def __init__(self, scene_ptr, object, type):
		self.scene_ptr = scene_ptr
		self.object = object
		self.type = type
		self.cr_idx = _lib.instance_new(self.scene_ptr, self.object.cr_idx, self.type)

	def set_transform(self, matrix):
		self.matrix = matrix
		_lib.instance_set_transform(self.scene_ptr, self.cr_idx, self.matrix)

	def transform(self, matrix):
		# TODO: Figure out matmul in python
		# self.matrix = self.matrix * matrix
		_lib.instance_set_transform(self.scene_ptr, self.cr_idx, self.matrix)

	def bind_materials(self, material_set):
		_lib.instance_bind_material_set(self.scene_ptr, self.cr_idx, material_set.cr_idx)

class vertex_buf:
	def __init__(self, scene_ptr, v, vn, n, nn, t, tn):
		self.cr_ptr = scene_ptr
		self.v = v
		self.vn = vn
		self.n = n
		self.nn = nn
		self.t = t
		self.tn = tn
		self.cr_idx = _lib.scene_vertex_buf_new(self.cr_ptr, self.v, self.vn, self.n, self.nn, self.t, self.tn)

class material_set:
	def __init__(self, scene_ptr):
		self.scene_ptr = scene_ptr
		self.materials = []
		self.cr_idx = _lib.scene_new_material_set(self.scene_ptr)

	def add(self, material):
		if material is None:
			_lib.material_set_add(self.scene_ptr, self.cr_idx, None)
			return
		self.materials.append(material)
		ct.pythonapi.PyCapsule_New.argtypes = [ct.c_void_p, ct.c_char_p, ct.c_void_p]
		ct.pythonapi.PyCapsule_New.restype = ct.py_object
		name = b'cray.shader_node'
		capsule = ct.pythonapi.PyCapsule_New(ct.byref(material.cr_struct), name, None)
		_lib.material_set_add(self.scene_ptr, self.cr_idx, capsule)

class scene:
	def __init__(self, scene_ptr):
		self.cr_ptr = scene_ptr
	def close(self):
		del(self.s_ptr)

	def totals(self):
		return _lib.scene_totals(self.cr_ptr)
	def mesh_new(self, name):
		return mesh(self.cr_ptr, name)
	def sphere_new(self, radius):
		return sphere(self.cr_ptr, radius)
	def camera_new(self):
		return camera(self.cr_ptr)
	def material_set_new(self):
		return material_set(self.cr_ptr)
	def instance_new(self, object, type):
		return instance(self.cr_ptr, object, type)
	def set_background(self, material):
		if material is None:
			return _lib.scene_set_background(self.cr_ptr, material)
		self.background = material
		ct.pythonapi.PyCapsule_New.argtypes = [ct.c_void_p, ct.c_char_p, ct.c_void_p]
		ct.pythonapi.PyCapsule_New.restype = ct.py_object
		name = b'cray.shader_node'
		capsule = ct.pythonapi.PyCapsule_New(ct.byref(material.cr_struct), name, None)
		return _lib.scene_set_background(self.cr_ptr, capsule)
	def vertex_buf_new(self, v, vn, n, nn, t, tn):
		return vertex_buf(self.cr_ptr, v, vn, n, nn, t, tn)

class cr_cb_info(ct.Structure):
	_fields_ = [
		("fb", ct.POINTER(cr_bitmap)),
		("tiles", ct.POINTER(ct.c_void_p)), # TODO
		("tiles_count", ct.c_size_t),
		("active_threads", ct.c_size_t),
		("avg_per_ray_us", ct.c_double),
		("samples_per_sec", ct.c_int64),
		("eta_ms", ct.c_int64),
		("completion", ct.c_double),
		("paused", ct.c_bool),
	]

cr_cb_func = ct.CFUNCTYPE(ct.c_void_p, ct.POINTER(cr_cb_info), ct.POINTER(ct.c_void_p))

class renderer:
	def __init__(self, path = None):
		self.obj_ptr = _lib.new_renderer()
		self.prefs = _pref(self.obj_ptr)
		if path != None:
			_lib.load_json(self.obj_ptr, path)

	def close(self):
		del(self.obj_ptr)

	def set_callback(self, type, callback_fn, user_data):
		if not callable(callback_fn):
			raise TypeError("callback_fn not callable")
		_lib.renderer_set_callback(self.obj_ptr, type, cr_cb_func(callback_fn), user_data)

	def stop():
		_lib.renderer_stop(self.obj_ptr)

	def toggle_pause():
		_lib.renderer_toggle_pause(self.obj_ptr)

	def render(self):
		_lib.renderer_render(self.obj_ptr)

	def get_result(self):
		ret = _lib.renderer_get_result(self.obj_ptr)
		# I saw this in several places, and suggested by a token predictor. Ehh?
		ct.pythonapi.PyCapsule_GetPointer.restype = ct.c_void_p
		ct.pythonapi.PyCapsule_GetPointer.argtypes = [ct.py_object, ct.c_char_p]
		ptr = ct.pythonapi.PyCapsule_GetPointer(ret, "cray.cr_bitmap".encode())
		ret_bitmap = ct.cast(ptr, ct.POINTER(cr_bitmap)).contents
		return ret_bitmap

	def scene_get(self):
		return scene(_lib.renderer_scene_get(self.obj_ptr))

	def debug_dump(self):
		_lib.debug_dump_state(self.obj_ptr)

	@classmethod
	def from_param(cls, param):
		if not isinstance(param, cls):
			raise TypeError("Expected an instance of Renderer")
		return param.obj_ptr

	@contextmanager
	def __call__(self):
		yield self
		self.close()
	def __enter__(self):
		return self
	def __exit__(self, exc_type, exc_value, traceback):
		self.close()

def start_render_worker(port, thread_limit):
	_lib.start_render_worker(port, thread_limit)

def send_shutdown_to_workers(node_list):
	_lib.send_shutdown_to_workers(node_list)
