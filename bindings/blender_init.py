
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

import bpy

from . import (
	c_ray
)

import ctypes as ct

from bpy.props import IntProperty
from bpy.props import PointerProperty

class CrayRenderSettings(bpy.types.PropertyGroup):
	samples: IntProperty(
		name="Samples",
		description="Number of samples to render for each pixel",
		min=1, max=(1 << 24),
		default=1,
	)
	bounces: IntProperty(
		name="Bounces",
		description="Max number of bounces for each light path",
		min=0, max=1024,
		default=12,
	)
	tile_size: IntProperty(
		name="Tile Size",
		description="Size of tile for each thread",
		min=0, max=1024,
		default=12,
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
	cr_face.mat_idx = 0
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
	print(len(normals))
	cr_vbuf = scene.vertex_buf_new(bytearray(vbuf), len(verts), bytearray(nbuf), len(normals), bytearray(tbuf), len(texcoords))
	return cr_vbuf

class CrayRender(bpy.types.RenderEngine):
	bl_idname = "c-ray"
	bl_label = "c-ray integration for Blender"
	bl_use_preview = False

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
			euler = mtx.to_euler('ZXY')
			loc = mtx.to_translation()
			cr_cam.set_param(c_ray.cam_param.fov, 80) #todo
			cr_cam.set_param(c_ray.cam_param.pose_x, loc[0])
			cr_cam.set_param(c_ray.cam_param.pose_y, loc[1])
			cr_cam.set_param(c_ray.cam_param.pose_z, loc[2])
			cr_cam.set_param(c_ray.cam_param.pose_roll, euler[0])
			cr_cam.set_param(c_ray.cam_param.pose_pitch, euler[1])
			cr_cam.set_param(c_ray.cam_param.pose_yaw, euler[2])
			cr_cam.set_param(c_ray.cam_param.res_x, self.size_x)
			cr_cam.set_param(c_ray.cam_param.res_y, self.size_y)
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
			cr_mat_set.add(None)
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
			mesh_triangulate(me)
			verts = me.vertices[:]
			me.calc_normals_split()
			faces = []
			for poly in me.polygons:
				faces.append(to_cr_face(me, poly))
			facebuf = (c_ray.cr_face * len(faces))(*faces)
			cr_mesh.bind_faces(bytearray(facebuf), len(faces))
			cr_mesh.bind_vertex_buf(cr_vertex_buf(cr_scene, me))
		return cr_scene

	def render(self, depsgraph):
		b_scene = depsgraph.scene
		scale = b_scene.render.resolution_percentage / 100.0
		self.size_x = int(b_scene.render.resolution_x * scale)
		self.size_y = int(b_scene.render.resolution_y * scale)

		renderer = c_ray.renderer()
		cr_scene = self.sync_scene(renderer, depsgraph, b_scene)
		renderer.prefs.output_path = "/tmp/"
		print(cr_scene.totals())
		renderer.debug_dump()
		del(renderer)
		# self.render_scene(scene)

	def test_render_scene(self, scene):
		pixel_count = self.size_x * self.size_y
		blueRect = [[0.0, 0.0, 1.0, 1.0]] * pixel_count
		result = self.begin_result(0, 0, self.size_x, self.size_y)
		layer = result.layers[0].passes["Combined"]
		layer.rect = blueRect
		self.end_result(result)

def get_panels():
	exclude_panels = {
		'VIEWLAYER_PT_filter',
		'VIEWLAYER_PT_layer_passes',
	}

	panels = []
	for panel in bpy.types.Panel.__subclasses__():
		if hasattr(panel, 'COMPAT_ENGINES') and 'BLENDER_RENDER' in panel.COMPAT_ENGINES:
			if panel.__name__ not in exclude_panels:
				panels.append(panel)

	return panels

def register():
	import faulthandler
	faulthandler.enable()
	print("Register libc-ray version {} ({})".format(c_ray.version.semantic, c_ray.version.githash))
	bpy.utils.register_class(CrayRender)
	bpy.utils.register_class(CrayRenderSettings)

	from bl_ui import (
		properties_render,
		properties_material,
	)
	# properties_render.RENDER_PT_render.COMPAT_ENGINES.add(CrayRender.bl_idname)
	properties_material.MATERIAL_PT_preview.COMPAT_ENGINES.add(CrayRender.bl_idname)
	for panel in get_panels():
		panel.COMPAT_ENGINES.add('C_RAY')
    
def unregister():
	print("Unregister libc-ray version {} ({})".format(c_ray.version.semantic, c_ray.version.githash))
	bpy.utils.unregister_class(CrayRenderSettings)
	bpy.utils.unregister_class(CrayRender)

	from bl_ui import (
		properties_render,
		properties_material,
	)
	# properties_render.RENDER_PT_render.COMPAT_ENGINES.remove(CrayRender.bl_idname)
	properties_material.MATERIAL_PT_preview.COMPAT_ENGINES.remove(CrayRender.bl_idname)
	for panel in get_panels():
		if 'C_RAY' in panel.COMPAT_ENGINES:
			panel.COMPAT_ENGINES.remove('C_RAY')
    
if __name__ == "__main__":
	register()
