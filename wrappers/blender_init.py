import bpy

class CrayRenderEngine(bpy.types.RenderEngine):
	bl_idname = "C-ray"
	bl_label = "C-ray path tracing integration"
	bl_use_preview = False

	def render(self, scene):
		scale = scene.render.resolution_percentage / 100.0
		self.size_x = int(scene.render.resolution_x * scale)
		self.size_y = int(scene.render.resolution_y * scale)
		self.render_scene(scene)

	def render_scene(self, scene):
		pixel_count = self.size_x * self.size_y
		blueRect = [[0.0, 0.0, 1.0, 1.0]] * pixel_count
		result = self.begin_result(0, 0, self.size_x, self.size_y)
		layer = result.layers[0].passes["Combined"]
		layer.rect = blueRect
		self.end_result(result)

def register():
	bpy.utils.register_class(CrayRenderEngine)

	from bl_ui import (
		properties_render,
		properties_material,
	)
	properties_render.RENDER_PT_render.COMPAT_ENGINES.add(CrayRenderEngine.bl_idname)
	properties_material.MATERIAL_PT_preview.COMPAT_ENGINES.add(CrayRenderEngine.bl_idname)
    
def unregister():
	bpy.utils.unregister_class(CrayRenderEngine)

	from bl_ui import (
		properties_render,
		properties_material,
	)
	properties_render.RENDER_PT_render.COMPAT_ENGINES.remove(CrayRenderEngine.bl_idname)
	properties_material.MATERIAL_PT_preview.COMPAT_ENGINES.remove(CrayRenderEngine.bl_idname)
    
if __name__ == "__main__":
	register()
