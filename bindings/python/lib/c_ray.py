import ctypes as ct
from contextlib import contextmanager
from enum import IntEnum

from . import cray_wrap as _lib
from . cray_wrap import *

class cr_renderer(ct.Structure):
	pass

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

class _cr_cb_type(IntEnum):
	on_start = 0,
	on_stop = 1,
	on_status_update = 2,
	on_state_changed = 3, # Not connected currently, c-ray never calls this
	on_interactive_pass_finished = 4

class _callbacks:
	def __init__(self, r_ptr):
		self.r_ptr = r_ptr

	def _set_on_start(self, fn_and_userdata):
		fn, user_data = fn_and_userdata
		if not callable(fn):
			raise TypeError("on_start callback function not callable")
		_lib.renderer_set_callback(self.r_ptr, _cr_cb_type.on_start, fn, user_data)
	on_start = property(None, _set_on_start, None, "Tuple (fn,user_data) - fn will be called when c-ray starts rendering, with arguments (cr_cb_info, user_data)")

	def _set_on_stop(self, fn_and_userdata):
		fn, user_data = fn_and_userdata
		if not callable(fn):
			raise TypeError("on_stop callback function not callable")
		_lib.renderer_set_callback(self.r_ptr, _cr_cb_type.on_stop, fn, user_data)
	on_stop = property(None, _set_on_stop, None, "Tuple (fn,user_data) - fn will be called when c-ray is done rendering, with arguments (cr_cb_info, user_data)")

	def _set_on_status_update(self, fn_and_userdata):
		fn, user_data = fn_and_userdata
		if not callable(fn):
			raise TypeError("on_status_update callback function not callable")
		_lib.renderer_set_callback(self.r_ptr, _cr_cb_type.on_status_update, fn, user_data)
	on_status_update = property(None, _set_on_status_update, None, "Tuple (fn,user_data) - fn will be called periodically while c-ray is rendering, with arguments (cr_cb_info, user_data)")

	def _set_on_interactive_pass_finished(self, fn_and_userdata):
		fn, user_data = fn_and_userdata
		if not callable(fn):
			raise TypeError("on_interactive_pass_finished callback function not callable")
		_lib.renderer_set_callback(self.r_ptr, _cr_cb_type.on_interactive_pass_finished, fn, user_data)
	on_interactive_pass_finished = property(None, _set_on_interactive_pass_finished, None, "Tuple (fn,user_data) - fn will be called every time c-ray finishes rendering a pass in interactive mode, with arguments (cr_cb_info, user_data)")

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
		self.instances = []
		self.cr_idx = _lib.scene_mesh_new(self.scene_ptr, self.name)

	# FIXME: me is a blender-specific object here, we shouldn't have Blender
	# stuff in our generic Python API!
	def bind_vertex_buf(self, me):
		num_verts = len(me.vertices)
		if num_verts < 1:
			print('Mesh \'{}\' has no vertices, bailing'.format(me.name))
			return
		ptr_v = ct.cast(me.vertices[0].as_pointer(), ct.POINTER(ct.c_float))
		name_v = b'cray.cr_vector'
		capsule_v = ct.pythonapi.PyCapsule_New(ptr_v, name_v, None)

		capsule_n = None
		num_normals = len(me.corner_normals)
		if num_normals > 0:
			ptr_n = ct.cast(me.corner_normals[0].as_pointer(), ct.POINTER(ct.c_float))
			capsule_n = ct.pythonapi.PyCapsule_New(ptr_n, name_v, None)

		num_texcoords = 0
		capsule_t = None
		num_uv_layers = len(me.uv_layers)
		if num_uv_layers > 0:
			# TODO: Do we dump all of these, or just the first one?
			layer = me.uv_layers[0]
			num_texcoords = len(layer.uv)
			ptr_t = ct.cast(layer.uv[0].as_pointer(), ct.POINTER(ct.c_float))
			name_t = b'cray.cr_coord'
			capsule_t = ct.pythonapi.PyCapsule_New(ptr_t, name_t, None)

		_lib.mesh_bind_vertex_buf(self.scene_ptr, self.cr_idx, capsule_v, num_verts, capsule_n, num_normals, capsule_t, num_texcoords)

	def bind_faces(self, faces, face_count):
		_lib.mesh_bind_faces(self.scene_ptr, self.cr_idx, faces, face_count)

	def finalize(self):
		_lib.mesh_finalize(self.scene_ptr, self.cr_idx)

	def instance_new(self):
		self.instances.append(instance(self.scene_ptr, self, 0))
		return self.instances[-1]

class sphere:
	def __init__(self, scene_ptr, radius):
		self.scene_ptr = scene_ptr
		self.radius = radius
		self.cr_idx = _lib.scene_add_sphere(self.scene_ptr, self.radius)

class _cam_param(IntEnum):
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

def _cam_get_num(scene_ptr, cam_idx, param):
	return _lib.camera_get_num_pref(scene_ptr, cam_idx, param)

def _cam_set_num(scene_ptr, cam_idx, param, value):
	_lib.camera_set_num_pref(scene_ptr, cam_idx, param, value)
	# Weird. Could just do this internally, no?
	_lib.camera_update(scene_ptr, cam_idx)

class _cam_pref:
	def __init__(self, scene_ptr, cam_idx):
		self.scene_ptr = scene_ptr
		self.cam_idx = cam_idx

	def _get_fov(self):
		return _cam_get_num(self.scene_ptr, self.cam_idx, _cam_param.fov)
	def _set_fov(self, value):
		return _cam_set_num(self.scene_ptr, self.cam_idx, _cam_param.fov, value)
	fov = property(_get_fov, _set_fov, None, "Camera field of view, in radians")

	def _get_focus_distance(self):
		return _cam_get_num(self.scene_ptr, self.cam_idx, _cam_param.focus_distance)
	def _set_focus_distance(self, value):
		return _cam_set_num(self.scene_ptr, self.cam_idx, _cam_param.focus_distance, value)
	focus_distance = property(_get_focus_distance, _set_focus_distance, None, "Camera focus distance, in meters")

	def _get_fstops(self):
		return _cam_get_num(self.scene_ptr, self.cam_idx, _cam_param.fstops)
	def _set_fstops(self, value):
		return _cam_set_num(self.scene_ptr, self.cam_idx, _cam_param.fstops, value)
	fstops = property(_get_fstops, _set_fstops, None, "Camera aperture, in f-stops")

	def _get_pose_x(self):
		return _cam_get_num(self.scene_ptr, self.cam_idx, _cam_param.pose_x)
	def _set_pose_x(self, value):
		return _cam_set_num(self.scene_ptr, self.cam_idx, _cam_param.pose_x, value)
	pose_x = property(_get_pose_x, _set_pose_x, None, "Camera x coordinate in world space")

	def _get_pose_y(self):
		return _cam_get_num(self.scene_ptr, self.cam_idx, _cam_param.pose_y)
	def _set_pose_y(self, value):
		return _cam_set_num(self.scene_ptr, self.cam_idx, _cam_param.pose_y, value)
	pose_y = property(_get_pose_y, _set_pose_y, None, "Camera y coordinate in world space")

	def _get_pose_z(self):
		return _cam_get_num(self.scene_ptr, self.cam_idx, _cam_param.pose_z)
	def _set_pose_z(self, value):
		return _cam_set_num(self.scene_ptr, self.cam_idx, _cam_param.pose_z, value)
	pose_z = property(_get_pose_z, _set_pose_z, None, "Camera z coordinate in world space")

	def _get_pose_roll(self):
		return _cam_get_num(self.scene_ptr, self.cam_idx, _cam_param.pose_roll)
	def _set_pose_roll(self, value):
		return _cam_set_num(self.scene_ptr, self.cam_idx, _cam_param.pose_roll, value)
	pose_roll = property(_get_pose_roll, _set_pose_roll, None, "Camera roll, in radians")

	def _get_pose_pitch(self):
		return _cam_get_num(self.scene_ptr, self.cam_idx, _cam_param.pose_pitch)
	def _set_pose_pitch(self, value):
		return _cam_set_num(self.scene_ptr, self.cam_idx, _cam_param.pose_pitch, value)
	pose_pitch = property(_get_pose_pitch, _set_pose_pitch, None, "Camera pitch, in radians")

	def _get_pose_yaw(self):
		return _cam_get_num(self.scene_ptr, self.cam_idx, _cam_param.pose_yaw)
	def _set_pose_yaw(self, value):
		return _cam_set_num(self.scene_ptr, self.cam_idx, _cam_param.pose_yaw, value)
	pose_yaw = property(_get_pose_yaw, _set_pose_yaw, None, "Camera yaw, in radians")

	def _get_time(self):
		return _cam_get_num(self.scene_ptr, self.cam_idx, _cam_param.time)
	def _set_time(self, value):
		return _cam_set_num(self.scene_ptr, self.cam_idx, _cam_param.time, value)
	time = property(_get_time, _set_time, None, "Camera animation t")

	def _get_res_x(self):
		return _cam_get_num(self.scene_ptr, self.cam_idx, _cam_param.res_x)
	def _set_res_x(self, value):
		return _cam_set_num(self.scene_ptr, self.cam_idx, _cam_param.res_x, value)
	res_x = property(_get_res_x, _set_res_x, None, "Camera x resolution, in pixels")
	def _get_res_y(self):
		return _cam_get_num(self.scene_ptr, self.cam_idx, _cam_param.res_y)
	def _set_res_y(self, value):
		return _cam_set_num(self.scene_ptr, self.cam_idx, _cam_param.res_y, value)
	res_y = property(_get_res_y, _set_res_y, None, "Camera y resolution, in pixels")
	def _get_blender_coord(self):
		return _cam_get_num(self.scene_ptr, self.cam_idx, _cam_param.blender_coord)
	def _set_blender_coord(self, value):
		return _cam_set_num(self.scene_ptr, self.cam_idx, _cam_param.blender_coord, value)
	blender_coord = property(_get_blender_coord, _set_blender_coord, None, "Boolean toggle to use Blender coordinate system in c-ray")
class camera:
	def __init__(self, scene_ptr):
		self.scene_ptr = scene_ptr
		self.cr_idx = _lib.camera_new(self.scene_ptr)
		self.opts = _cam_pref(self.scene_ptr, self.cr_idx)
		self.params = {}

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
		self.material_set = None
		self.matrix = None
		self.cr_idx = _lib.instance_new(self.scene_ptr, self.object.cr_idx, self.type)

	def set_transform(self, matrix):
		if self.matrix == matrix:
			return
		self.matrix = matrix
		_lib.instance_set_transform(self.scene_ptr, self.cr_idx, self.matrix)

	def transform(self, matrix):
		# TODO: Figure out matmul in python
		# self.matrix = self.matrix * matrix
		_lib.instance_set_transform(self.scene_ptr, self.cr_idx, self.matrix)

	def bind_materials(self, material_set):
		self.material_set = material_set
		_lib.instance_bind_material_set(self.scene_ptr, self.cr_idx, self.material_set.cr_idx)

class material_set:
	def __init__(self, scene_ptr):
		self.scene_ptr = scene_ptr
		self.materials = {}
		self.cr_idx = _lib.scene_new_material_set(self.scene_ptr)

	def add(self, material, name):
		if material is None:
			idx = _lib.material_set_add(self.scene_ptr, self.cr_idx, None)
			self.materials[name] = idx
			return
		ct.pythonapi.PyCapsule_New.argtypes = [ct.c_void_p, ct.c_char_p, ct.c_void_p]
		ct.pythonapi.PyCapsule_New.restype = ct.py_object
		capsule_name = b'cray.shader_node'
		capsule = ct.pythonapi.PyCapsule_New(ct.byref(material.cr_struct), capsule_name, None)
		idx = _lib.material_set_add(self.scene_ptr, self.cr_idx, capsule)
		if name in self.materials:
			print("ADD DUPLICATE matset {} idx {} ({})".format(self.cr_idx, self.materials[name], name))
		self.materials[name] = idx

	def update(self, matname, new):
		if new is None:
			return
		ct.pythonapi.PyCapsule_New.argtypes = [ct.c_void_p, ct.c_char_p, ct.c_void_p]
		ct.pythonapi.PyCapsule_New.restype = ct.py_object
		capsule_name = b'cray.shader_node'
		capsule = ct.pythonapi.PyCapsule_New(ct.byref(new.cr_struct), capsule_name, None)
		_lib.material_update(self.scene_ptr, self.cr_idx, self.materials[matname], capsule)
		print("matset {} material {} ({}) updated".format(self.cr_idx, matname, self.materials[matname]))

class scene:
	def __init__(self, cr_renderer):
		self.cr_renderer = cr_renderer
		self.cr_ptr = _lib.renderer_scene_get(self.cr_renderer)
		self.meshes = {}
		self.cameras = {}
		self.material_sets = {}
	def close(self):
		del(self.s_ptr)

	def totals(self):
		return _lib.scene_totals(self.cr_ptr)
	def mesh_new(self, name):
		self.meshes[name] = mesh(self.cr_ptr, name)
		return self.meshes[name]
	def sphere_new(self, radius):
		return sphere(self.cr_ptr, radius)
	def camera_new(self, name):
		self.cameras[name] = camera(self.cr_ptr)
		return self.cameras[name]
	def material_set_new(self, name):
		self.material_sets[name] = material_set(self.cr_ptr)
		return self.material_sets[name]
	def set_background(self, material):
		if material is None:
			return _lib.scene_set_background(self.cr_ptr, material)
		self.background = material
		ct.pythonapi.PyCapsule_New.argtypes = [ct.c_void_p, ct.c_char_p, ct.c_void_p]
		ct.pythonapi.PyCapsule_New.restype = ct.py_object
		name = b'cray.shader_node'
		capsule = ct.pythonapi.PyCapsule_New(ct.byref(material.cr_struct), name, None)
		return _lib.scene_set_background(self.cr_ptr, capsule)

class renderer:
	ret_bitmap = None
	def __init__(self, path = None):
		self.obj_ptr = _lib.new_renderer()
		self.prefs = _pref(self.obj_ptr)
		self.callbacks = _callbacks(self.obj_ptr)
		self.interactive = False
		if path != None:
			_lib.load_json(self.obj_ptr, path)

	def close(self):
		_lib.renderer_destroy(self.obj_ptr)
		del(self.obj_ptr)

	def stop(self):
		self.interactive = False
		_lib.renderer_stop(self.obj_ptr)

	def restart(self):
		_lib.renderer_restart(self.obj_ptr)

	def toggle_pause():
		_lib.renderer_toggle_pause(self.obj_ptr)

	def render(self):
		_lib.renderer_render(self.obj_ptr)

	def start_interactive(self):
		self.interactive = True
		_lib.renderer_start_interactive(self.obj_ptr)

	def get_result(self):
		# TODO: Would also be nice if the result object as well as renderer.prefs would have dims as a tuple/array? I think that's a nicer way to express those in python.
		if self.ret_bitmap is not None:
			return self.ret_bitmap
		self.ret_bitmap = _lib.renderer_get_result(self.obj_ptr)
		return self.ret_bitmap

	def scene_get(self):
		return scene(self.obj_ptr)

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


class log_level(IntEnum):
	Silent = 0
	Info = 1
	Debug = 2
	Spam = 3

def log_level_set(level):
	_lib.log_level_set(level)

def log_level_get():
	return _lib.log_level_get()
