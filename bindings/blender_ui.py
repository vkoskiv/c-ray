import bpy
from bpy.types import Panel

# Most of this is just a carbon-copy of the Cycles UI boilerplate

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

class CRAY_PT_context_material(CrayButtonsPanel, Panel):
    bl_label = ""
    bl_context = "material"
    bl_options = {'HIDE_HEADER'}

    @classmethod
    def poll(cls, context):
        if context.active_object and context.active_object.type == 'GPENCIL':
            return False
        else:
            return (context.material or context.object) and CrayButtonsPanel.poll(context)

    def draw(self, context):
        layout = self.layout

        mat = context.material
        ob = context.object
        slot = context.material_slot
        space = context.space_data

        if ob:
            is_sortable = len(ob.material_slots) > 1
            rows = 3
            if (is_sortable):
                rows = 4

            row = layout.row()

            row.template_list("MATERIAL_UL_matslots", "", ob, "material_slots", ob, "active_material_index", rows=rows)

            col = row.column(align=True)
            col.operator("object.material_slot_add", icon='ADD', text="")
            col.operator("object.material_slot_remove", icon='REMOVE', text="")
            col.separator()
            col.menu("MATERIAL_MT_context_menu", icon='DOWNARROW_HLT', text="")

            if is_sortable:
                col.separator()

                col.operator("object.material_slot_move", icon='TRIA_UP', text="").direction = 'UP'
                col.operator("object.material_slot_move", icon='TRIA_DOWN', text="").direction = 'DOWN'

            if ob.mode == 'EDIT':
                row = layout.row(align=True)
                row.operator("object.material_slot_assign", text="Assign")
                row.operator("object.material_slot_select", text="Select")
                row.operator("object.material_slot_deselect", text="Deselect")

        row = layout.row()

        if ob:
            row.template_ID(ob, "active_material", new="material.new")

            if slot:
                icon_link = 'MESH_DATA' if slot.link == 'DATA' else 'OBJECT_DATA'
                row.prop(slot, "link", text="", icon=icon_link, icon_only=True)

        elif mat:
            layout.template_ID(space, "pin_id")
            layout.separator()

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
	CRAY_PT_context_material,
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
