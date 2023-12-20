import bpy
from bpy.types import Panel

class CrayButtonsPanel:
	bl_space_type = "PROPERTIES"
	bl_region_type = "WINDOW"
	bl_context = "render"
	COMPAT_ENGINES = {'C_RAY'}
	@classmethod
	def poll(cls, context):
		return context.engine in cls.COMPAT_ENGINES

class C_RAY_RENDER_PT_sampling(CrayButtonsPanel, Panel):
	bl_label = "Sampling"

	def draw(self, context):
		pass

class C_RAY_RENDER_PT_sampling_render(CrayButtonsPanel, Panel):
	bl_label = "Render"
	bl_parent_id = "C_RAY_RENDER_PT_sampling"

	def draw(self, context):
		layout = self.layout
		layout.use_property_split = True
		layout.use_property_decorate = False

		heading = layout.column(align=True, heading="Samples")
		row = heading.row(align=True)
		row.prop(context.scene.c_ray, "samples", text="Samples")

class C_RAY_RENDER_PT_performance(CrayButtonsPanel, Panel):
	bl_label = "Performance"
	bl_options = {'DEFAULT_CLOSED'}

	def draw(self, context):
		pass

class C_RAY_RENDER_PT_performance_threads(CrayButtonsPanel, Panel):
	bl_label = "Threads"
	bl_parent_id = "C_RAY_RENDER_PT_performance"

	def draw(self, context):
		layout = self.layout
		layout.use_property_split = True
		layout.use_property_decorate = False

		col = layout.column()
		col.prop(context.scene.c_ray, "threads")

class C_RAY_RENDER_PT_performance_tiling(CrayButtonsPanel, Panel):
	bl_label = "Tiling"
	bl_parent_id = "C_RAY_RENDER_PT_performance"

	def draw(self, context):
		layout = self.layout
		layout.use_property_split = True
		layout.use_property_decorate = False

		col = layout.column()
		col.prop(context.scene.c_ray, "tile_size")

class C_RAY_RENDER_PT_performance_clustering(CrayButtonsPanel, Panel):
	bl_label = "Clustering"
	bl_parent_id = "C_RAY_RENDER_PT_performance"

	def draw(self, context):
		layout = self.layout
		layout.use_property_split = True
		layout.use_property_decorate = False

		col = layout.column()
		col.prop(context.scene.c_ray, "node_list", text="Worker node list")

class C_RAY_RENDER_PT_light_paths(CrayButtonsPanel, Panel):
	bl_label = "Light Paths"
	bl_options = {'DEFAULT_CLOSED'}

	def draw(self, context):
		pass

class C_RAY_RENDER_PT_light_paths_bounces(CrayButtonsPanel, Panel):
	bl_label = "Bounces"
	bl_parent_id = "C_RAY_RENDER_PT_light_paths"

	def draw(self, context):
		layout = self.layout
		layout.use_property_split = True
		layout.use_property_decorate = False

		col = layout.column()
		col.prop(context.scene.c_ray, "bounces", text="Total")

class C_RAY_MATERIAL_PT_preview(CrayButtonsPanel, Panel):
	bl_label = "Preview"
	bl_context = "material"
	bl_options = {'DEFAULT_CLOSED'}

	@classmethod
	def poll(cls, context):
		mat = context.material
		return mat and (not mat.grease_pencil) and CrayButtonsPanel.poll(context)

	def draw(self, context):
		self.layout.template_preview(context.material)

class C_RAY_CAMERA_PT_dof(CrayButtonsPanel, Panel):
	bl_label = "Depth of Field"
	bl_context = "data"

	@classmethod
	def poll(cls, context):
		return context.camera and CrayButtonsPanel.poll(context)

	def draw_header(self, context):
		cam = context.camera
		dof = cam.dof
		self.layout.prop(dof, "use_dof", text="")

	def draw(self, context):
		layout = self.layout
		layout.use_property_split = True

		cam = context.camera
		dof = cam.dof
		layout.active = dof.use_dof

		split = layout.split()

		col = split.column()
		col.prop(dof, "focus_object", text="Focus Object")
		if dof.focus_object and dof.focus_object.type == 'ARMATURE':
			col.prop_search(dof, "focus_subtarget", dof.focus_object.data, "bones", text="Focus Bone")
		sub = col.row()
		sub.active = dof.focus_object is None
		sub.prop(dof, "focus_distance", text="Distance")

class C_RAY_CAMERA_PT_dof_aperture(CrayButtonsPanel, Panel):
	bl_label = "Aperture"
	bl_parent_id = "C_RAY_CAMERA_PT_dof"

	@classmethod
	def poll(cls, context):
		return context.camera and CrayButtonsPanel.poll(context)

	def draw(self, context):
		layout = self.layout
		layout.use_property_split = True

		cam = context.camera
		dof = cam.dof
		layout.active = dof.use_dof
		flow = layout.grid_flow(row_major=True, columns=0, even_columns=True, even_rows=False, align=False)

		col = flow.column()
		col.prop(dof, "aperture_fstop")

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

classes = (
	C_RAY_RENDER_PT_sampling,
	C_RAY_RENDER_PT_sampling_render,
	C_RAY_RENDER_PT_performance,
	C_RAY_RENDER_PT_performance_threads,
	C_RAY_RENDER_PT_performance_tiling,
	C_RAY_RENDER_PT_performance_clustering,
	C_RAY_RENDER_PT_light_paths,
	C_RAY_RENDER_PT_light_paths_bounces,
	C_RAY_MATERIAL_PT_preview,
	C_RAY_CAMERA_PT_dof,
	C_RAY_CAMERA_PT_dof_aperture,
)

def register():
	from bpy.utils import register_class

	from bl_ui import (
		properties_render,
		properties_material,
	)
	# properties_render.RENDER_PT_render.COMPAT_ENGINES.add(CrayRender.bl_idname)
	# properties_material.MATERIAL_PT_preview.COMPAT_ENGINES.add(CrayRender.bl_idname)
	for panel in get_panels():
		panel.COMPAT_ENGINES.add('C_RAY')

	for cls in classes:
		register_class(cls)

def unregister():
	from bpy.utils import unregister_class

	from bl_ui import (
		properties_render,
		properties_material,
	)
	# properties_render.RENDER_PT_render.COMPAT_ENGINES.remove(CrayRender.bl_idname)
	# properties_material.MATERIAL_PT_preview.COMPAT_ENGINES.remove(CrayRender.bl_idname)
	for panel in get_panels():
		if 'C_RAY' in panel.COMPAT_ENGINES:
			panel.COMPAT_ENGINES.remove('C_RAY')

	for cls in classes:
		unregister_class(cls)
