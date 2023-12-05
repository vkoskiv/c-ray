import ctypes as ct
from contextlib import contextmanager
from enum import IntEnum

c_ray = ct.CDLL("lib/libc-ray.so")

c_ray.cr_get_version.restype = ct.c_char_p
def cr_get_version():
	return c_ray.cr_get_version().decode()

c_ray.cr_get_git_hash.restype = ct.c_char_p
def cr_get_git_hash():
	return c_ray.cr_get_git_hash().decode()

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

c_ray.cr_renderer_render.restype = ct.POINTER(cr_bitmap)
c_ray.cr_renderer_render.argtypes = [ct.POINTER(cr_renderer)]
c_ray.cr_bitmap_free.argtypes = [ct.POINTER(cr_bitmap)]

c_ray.cr_new_renderer.restype = ct.POINTER(cr_renderer)
c_ray.cr_destroy_renderer.argtypes = [ct.POINTER(cr_renderer)]

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
	should_save = 9
	override_cam = 10
	is_iterative = 11
	# str
	output_path = 12
	asset_path = 13
	output_name = 14
	output_filetype = 15
	node_list = 16

c_ray.cr_renderer_set_num_pref.restype = ct.c_bool
c_ray.cr_renderer_set_num_pref.argtypes = [ct.POINTER(cr_renderer), ct.c_int, ct.c_uint64]
def _r_set_num(ptr, param, value):
	return c_ray.cr_renderer_set_num_pref(ptr, param, value)

c_ray.cr_renderer_get_num_pref.restype = ct.c_uint64
c_ray.cr_renderer_get_num_pref.argtypes = [ct.POINTER(cr_renderer), ct.c_int]
def _r_get_num(ptr, param):
	return c_ray.cr_renderer_get_num_pref(ptr, param)

c_ray.cr_renderer_set_str_pref.restype = ct.c_bool
c_ray.cr_renderer_set_str_pref.argtypes = [ct.POINTER(cr_renderer), ct.c_int, ct.c_char_p]
def _r_set_str(ptr, param, value):
	return c_ray.cr_renderer_set_str_pref(ptr, param, value.encode())

c_ray.cr_renderer_get_str_pref.restype = ct.c_char_p
c_ray.cr_renderer_get_str_pref.argtypes = [ct.POINTER(cr_renderer), ct.c_int]
def _r_get_str(ptr, param):
	return c_ray.cr_renderer_get_str_pref(ptr, param).decode()

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

class renderer:
	def __init__(self):
		self.obj_ptr = c_ray.cr_new_renderer()
		self.prefs = _pref(self.obj_ptr)

	def close(self):
		c_ray.cr_destroy_renderer(self.obj_ptr)


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
