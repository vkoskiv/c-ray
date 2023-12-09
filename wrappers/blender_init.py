
bl_info = {
	"name": "c-ray integration for Blender",
	"author": "Valtteri Koskivuori",
	"blender": (2, 80, 0),
	"description": "Experimenting with the new c-ray Python API",
	"doc_url": "https://github.com/vkoskiv/c-ray",
	"tracker_url": "",
	"support": "UNOFFICIAL",
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

class CrayRender(bpy.types.RenderEngine):
	bl_idname = "c-ray"
	bl_label = "c-ray integration for Blender"
	bl_use_preview = False

	def sync_scene(renderer, depsgraph, b_scene):
		cr_scene = renderer.scene_get()
		objects = b_scene.objects
		for ob_main in enumerate(objects):
			cr_mesh = cr_scene.mesh_new(ob_main.name)
			instances = []
			new_inst = cr_scene.instance_new(cr_mesh, 0)
			new_inst.set_transform(ob_main.matrix_world)
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
				indices = []
				for loop_idx in range(poly.loop_start, poly.loop_start + poly.loop_total):
					indices.append(me.loops[loop_idx].vertex_index)
				face = c_ray.cr_face()
				face.vertex_idx = indices
				face.mat_idx = 0
				face.has_normals = 0
				faces.append(face)
			facebuf = (c_ray.cr_face * len(faces))(*faces)
			cr_mesh.bind_faces(bytearray(facebuf), len(faces))

	def render(self, depsgraph):
		b_scene = depsgraph.scene
		scale = b_scene.render.resolution_percentage / 100.0
		self.size_x = int(b_scene.render.resolution_x * scale)
		self.size_y = int(b_scene.render.resolution_y * scale)

		renderer = c_ray.renderer()
		cr_scene = self.sync_scene(renderer, depsgraph, b_scene)
		self.render_scene(scene)

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
	bpy.utils.register_class(CrayRenderEngine)
	bpy.utils.register_class(CrayRenderSettings)

	from bl_ui import (
		properties_render,
		properties_material,
	)
	properties_render.RENDER_PT_render.COMPAT_ENGINES.add(CrayRenderEngine.bl_idname)
	properties_material.MATERIAL_PT_preview.COMPAT_ENGINES.add(CrayRenderEngine.bl_idname)
	for panel in get_panels():
		panel.COMPAT_ENGINES.add('C_RAY')
    
def unregister():
	bpy.utils.unregister_class(CrayRenderSettings)
	bpy.utils.unregister_class(CrayRenderEngine)

	from bl_ui import (
		properties_render,
		properties_material,
	)
	properties_render.RENDER_PT_render.COMPAT_ENGINES.remove(CrayRenderEngine.bl_idname)
	properties_material.MATERIAL_PT_preview.COMPAT_ENGINES.remove(CrayRenderEngine.bl_idname)
	for panel in get_panels():
		if 'C_RAY' in panel.COMPAT_ENGINES:
			panel.COMPAT_ENGINES.remove('C_RAY')
    
if __name__ == "__main__":
	register()
