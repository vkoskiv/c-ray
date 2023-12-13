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
		scene = context.scene
		# I have no idea where I could patch in my own object for storing settings,
		# so I'm just reusing the Cycles one.
		cscene = scene.cycles
		layout.use_property_split = True
		layout.use_property_decorate = False

		heading = layout.column(align=True, heading="Samples")
		row = heading.row(align=True)
		row.prop(cscene, "samples", text="Samples")

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
	from bpy.utils import register_class

	from bl_ui import (
		properties_render,
		properties_material,
	)
	# properties_render.RENDER_PT_render.COMPAT_ENGINES.add(CrayRender.bl_idname)
	# properties_material.MATERIAL_PT_preview.COMPAT_ENGINES.add(CrayRender.bl_idname)
	for panel in get_panels():
		panel.COMPAT_ENGINES.add('C_RAY')

	register_class(C_RAY_RENDER_PT_sampling)
	register_class(C_RAY_RENDER_PT_sampling_render)

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

	unregister_class(C_RAY_RENDER_PT_sampling)
	unregister_class(C_RAY_RENDER_PT_sampling_render)
