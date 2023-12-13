import bpy

from bpy.props import (
	FloatProperty,
	PointerProperty
)

class CrayCameraSettings(bpy.types.PropertyGroup):
	fov: FloatProperty(
		name="Field of View",
		description="Field of view of the camera",
		min=1.0, max=160.0,
		subtype='ANGLE',
		default=80.0
	)
	@classmethod
	def register(cls):
		bpy.types.Camera.c_ray = PointerProperty(
			name="c-ray Camera Settings",
			description="c-ray Camera Settings",
			type=cls
		)

	@classmethod
	def unregister(cls):
		del bpy.types.Camera.c_ray

def register():
	bpy.utils.register_class(CrayCameraSettings)

def unregister():
	bpy.utils.unregister_class(CrayCameraSettings)
