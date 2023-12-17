
bl_info = {
	"name": "c-ray integration for Blender",
	"author": "Valtteri Koskivuori",
	"blender": (2, 80, 0),
	"description": "Experimenting with the new c-ray Python API",
	"doc_url": "https://github.com/vkoskiv/c-ray",
	"tracker_url": "",
	"category": "Render"
}

if "bpy" in locals():
	import importlib
	if "c_ray" in locals():
		importlib.reload(c_ray)
	if "properties" in locals():
		importlib.reload(properties)
	if "ui" in locals():
		importlib.reload(ui)

import bpy

from . import (
	c_ray
)

import ctypes as ct
from array import array
import math
import mathutils

from . nodes.color import (
	cr_color
)

from . nodes.convert import *

from bpy.props import (
	IntProperty,
	PointerProperty,
	StringProperty,
)

class CrayRenderSettings(bpy.types.PropertyGroup):
	samples: IntProperty(
		name="Samples",
		description="Number of samples to render for each pixel",
		min=1, max=(1 << 24),
		default=32,
	)
	threads: IntProperty(
		name="Threads",
		description="Number of threads to use for rendering",
		min=1, max=256,
		default=4,
	)
	tile_size: IntProperty(
		name="Tile Size",
		description="Size of tile for each thread",
		min=0, max=1024,
		default=32,
	)
	bounces: IntProperty(
		name="Bounces",
		description="Max number of bounces for each light path",
		min=0, max=1024,
		default=16,
	)
	node_list: StringProperty(
		name="Worker node list",
		description="comma-separated list of IP:port pairs",
		maxlen=4096,
		default=""
	)
	@classmethod
	def register(cls):
		bpy.types.Scene.c_ray = PointerProperty(
			name="c-ray Render Settings",
			description="c-ray Render Settings",
			type=cls,
		)

	@classmethod
	def unregister(cls):
		del bpy.types.Scene.c_ray

def mesh_triangulate(mesh):
	import bmesh
	bm = bmesh.new();
	bm.from_mesh(mesh)
	bmesh.ops.triangulate(bm, faces=bm.faces)
	bm.to_mesh(mesh)
	bm.free()

def to_cr_matrix(matrix):
	cr_mtx = c_ray.cr_matrix()
	cr_mtx.mtx[0] = matrix[0][0]
	cr_mtx.mtx[1] = matrix[0][1]
	cr_mtx.mtx[2] = matrix[0][2]
	cr_mtx.mtx[3] = matrix[0][3]
	cr_mtx.mtx[4] = matrix[1][0]
	cr_mtx.mtx[5] = matrix[1][1]
	cr_mtx.mtx[6] = matrix[1][2]
	cr_mtx.mtx[7] = matrix[1][3]
	cr_mtx.mtx[8] = matrix[2][0]
	cr_mtx.mtx[9] = matrix[2][1]
	cr_mtx.mtx[10] = matrix[2][2]
	cr_mtx.mtx[11] = matrix[2][3]
	cr_mtx.mtx[12] = matrix[3][0]
	cr_mtx.mtx[13] = matrix[3][1]
	cr_mtx.mtx[14] = matrix[3][2]
	cr_mtx.mtx[15] = matrix[3][3]
	return cr_mtx

def to_cr_face(me, poly):
	indices = []
	for loop_idx in range(poly.loop_start, poly.loop_start + poly.loop_total):
		indices.append(me.loops[loop_idx].vertex_index)
	cr_face = c_ray.cr_face()
	cr_face.vertex_idx[:] = indices
	cr_face.mat_idx = poly.material_index
	cr_face.has_normals = 0
	return cr_face

def cr_vertex_buf(scene, me):
	verts = []
	for v in me.vertices:
		cr_vert = c_ray.cr_vector()
		cr_vert.x = v.co[0]
		cr_vert.y = v.co[1]
		cr_vert.z = v.co[2]
		verts.append(cr_vert)
	normals = []
	texcoords = []
	vbuf = (c_ray.cr_vector * len(verts))(*verts)
	nbuf = (c_ray.cr_vector * len(verts))(*normals)
	tbuf = (c_ray.cr_coord  * len(verts))(*texcoords)
	print("new vbuf: v: {}, n: {}, t: {}".format(len(verts), len(normals), len(texcoords)))
	cr_vbuf = scene.vertex_buf_new(bytearray(vbuf), len(verts), bytearray(nbuf), len(normals), bytearray(tbuf), len(texcoords))
	return cr_vbuf

class CrayRender(bpy.types.RenderEngine):
	bl_idname = "C_RAY"
	bl_label = "c-ray for Blender"
	bl_use_preview = False
	bl_use_shading_nodes_custom = False

	def sync_scene(self, renderer, depsgraph, b_scene):
		cr_scene = renderer.scene_get()
		objects = b_scene.objects
		# Sync cameras
		for idx, ob_main in enumerate(objects):
			if ob_main.type != 'CAMERA':
				continue
			bl_cam_eval = ob_main.evaluated_get(depsgraph)
			bl_cam = bl_cam_eval.data

			cr_cam = cr_scene.camera_new()
			mtx = ob_main.matrix_world
			euler = mtx.to_euler('XYZ')
			loc = mtx.to_translation()
			cr_cam.set_param(c_ray.cam_param.fov, math.degrees(bl_cam.angle))
			cr_cam.set_param(c_ray.cam_param.pose_x, loc[0])
			cr_cam.set_param(c_ray.cam_param.pose_y, loc[1])
			cr_cam.set_param(c_ray.cam_param.pose_z, loc[2])
			cr_cam.set_param(c_ray.cam_param.pose_roll, euler[0])
			cr_cam.set_param(c_ray.cam_param.pose_pitch, euler[1])
			cr_cam.set_param(c_ray.cam_param.pose_yaw, euler[2])
			cr_cam.set_param(c_ray.cam_param.res_x, self.size_x)
			cr_cam.set_param(c_ray.cam_param.res_y, self.size_y)
			cr_cam.set_param(c_ray.cam_param.blender_coord, 1)
			# TODO: We don't tell blender that we support this, so it's not available yet
			if bl_cam.dof.use_dof:
				cr_cam.set_param(c_ray.cam_param.fstops, bl_cam.dof.aperture_fstop)
				cr_cam.set_param(c_ray.cam_param.focus_distance, bl_cam.dof.focus_distance)

		# Sync meshes
		for idx, ob_main in enumerate(objects):
			if ob_main.type != 'MESH':
				continue
			cr_mesh = cr_scene.mesh_new(ob_main.name)
			instances = []
			new_inst = cr_scene.instance_new(cr_mesh, 0)
			new_inst.set_transform(to_cr_matrix(ob_main.matrix_world))
			cr_mat_set = cr_scene.material_set_new()
			new_inst.bind_materials(cr_mat_set)
			instances.append(new_inst)
			if ob_main.is_instancer:
				for dup in depsgraph.object_instances:
					if dup.parent and dup.parent.original == ob_main:
						new_inst = cr_scene.instance_new(cr_mesh, 0)
						new_inst.set_transform(dup.matrix_world.copy())
			ob_for_convert = ob_main.evaluated_get(depsgraph)
			try:
				me = ob_for_convert.to_mesh()
			except RuntimeError:
				me = None
			if me is None:
				continue
			for bl_mat in me.materials:
				if bl_mat.use_nodes:
					cr_mat_set.add(convert_node_tree(depsgraph, bl_mat, bl_mat.node_tree))
				else:
					print("Material {} doesn't use nodes, do something about that".format(bl_mat.name))
					cr_mat_set.add(None)
			mesh_triangulate(me)
			verts = me.vertices[:]
			me.calc_normals_split()
			faces = []
			for poly in me.polygons:
				faces.append(to_cr_face(me, poly))
			facebuf = (c_ray.cr_face * len(faces))(*faces)
			cr_mesh.bind_faces(bytearray(facebuf), len(faces))
			cr_mesh.bind_vertex_buf(cr_vertex_buf(cr_scene, me))
		
		# Set background shader
		bl_nodetree = bpy.data.worlds[0].node_tree
		cr_scene.set_background(convert_background(bl_nodetree))
		return cr_scene

	def render(self, depsgraph):
		b_scene = depsgraph.scene
		scale = b_scene.render.resolution_percentage / 100.0
		self.size_x = int(b_scene.render.resolution_x * scale)
		self.size_y = int(b_scene.render.resolution_y * scale)

		renderer = c_ray.renderer()
		cr_scene = self.sync_scene(renderer, depsgraph, b_scene)
		print(cr_scene.totals())
		# renderer.debug_dump()
		renderer.prefs.samples = b_scene.c_ray.samples
		renderer.prefs.threads = b_scene.c_ray.threads
		renderer.prefs.tile_x = b_scene.c_ray.tile_size
		renderer.prefs.tile_y = b_scene.c_ray.tile_size
		renderer.prefs.bounces = b_scene.c_ray.bounces
		renderer.prefs.node_list = b_scene.c_ray.node_list
		renderer.prefs.blender_mode = True
		bm = renderer.render()
		self.display_bitmap(bm)
		del(renderer)

	def display_bitmap(self, bm):
		float_count = bm.width * bm.height * bm.stride
		floats = array('f', bm.data.float_ptr[:float_count])
		rect = []
		for i in range(0, len(floats), 3):
			temp = []
			temp.append(floats[i + 0])
			temp.append(floats[i + 1])
			temp.append(floats[i + 2])
			temp.append(1.0)
			rect.append(temp)
		result = self.begin_result(0, 0, bm.width, bm.height)
		layer = result.layers[0].passes["Combined"]
		layer.rect = rect
		self.end_result(result)

def register():
	from . import properties
	from . import ui
	import faulthandler
	faulthandler.enable()
	print("Register libc-ray version {} ({})".format(c_ray.version.semantic, c_ray.version.githash))
	properties.register()
	ui.register()
	bpy.utils.register_class(CrayRender)
	bpy.utils.register_class(CrayRenderSettings)

    
def unregister():
	from . import properties
	from . import ui
	print("Unregister libc-ray version {} ({})".format(c_ray.version.semantic, c_ray.version.githash))

	properties.unregister()
	ui.unregister()

	bpy.utils.unregister_class(CrayRenderSettings)
	bpy.utils.unregister_class(CrayRender)

    
if __name__ == "__main__":
	register()
