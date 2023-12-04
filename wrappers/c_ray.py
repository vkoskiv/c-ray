import ctypes as ct
from contextlib import contextmanager
from enum import IntEnum

c_ray = ct.CDLL("lib/libc-ray.so")

c_ray.cr_get_version.restype = ct.c_char_p
def cr_get_version():
    return c_ray.cr_get_version()

c_ray.cr_get_git_hash.restype = ct.c_char_p
def cr_get_git_hash():
    return c_ray.cr_get_git_hash()

class cr_renderer(ct.Structure):
    pass

c_ray.cr_new_renderer.restype = ct.POINTER(cr_renderer)
c_ray.cr_destroy_renderer.argtypes = [ct.POINTER(cr_renderer)]

class cr_renderer_param(IntEnum):
	cr_renderer_threads = 0
	cr_renderer_samples = 1
	cr_renderer_bounces = 2
	cr_renderer_tile_width = 3
	cr_renderer_tile_height = 4
	cr_renderer_tile_order = 5
	cr_renderer_output_path = 6
	cr_renderer_asset_path = 7
	cr_renderer_output_name = 8
	cr_renderer_output_filetype = 9
	cr_renderer_output_num = 10
	cr_renderer_override_width = 11
	cr_renderer_override_height = 12
	cr_renderer_should_save = 13
	cr_renderer_override_cam = 14
	cr_renderer_node_list = 15
	cr_renderer_is_iterative = 16

c_ray.cr_renderer_set_num_pref.restype = ct.c_bool
c_ray.cr_renderer_set_num_pref.argtypes = [ct.POINTER(cr_renderer), ct.c_int, ct.c_uint64]

c_ray.cr_renderer_get_num_pref.restype = ct.c_uint64
c_ray.cr_renderer_get_num_pref.argtypes = [ct.POINTER(cr_renderer), ct.c_int]

class Renderer:
    def __init__(self):
        self.obj_ptr = c_ray.cr_new_renderer()

    def close(self):
        c_ray.cr_destroy_renderer(self.obj_ptr)

    def set_num(self, param, val):
        if not isinstance(param, cr_renderer_param):
            raise TypeError("Expected an instance of cr_renderer_param")
        if not isinstance(val, ct.c_uint64):
            raise TypeError("Expected an instance of c_uint64")
        return c_ray.cr_renderer_set_num_pref(self.obj_ptr, param, val)

    def get_num(self, param):
        if not isinstance(param, cr_renderer_param):
            raise TypeError("Expected an instance of cr_renderer_param")
        return c_ray.cr_renderer_get_num_pref(self.obj_ptr, param)

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

if __name__ == "__main__":
    print("Hello, c-ray!");
    print("libc-ray version {} ({})".format(cr_get_version().decode(), cr_get_git_hash().decode()))
    with Renderer() as r:
        r.set_num(cr_renderer_param.cr_renderer_threads, ct.c_uint64(42))
        print("Got value {}".format(r.get_num(cr_renderer_param.cr_renderer_threads)))
        r.set_num(cr_renderer_param.cr_renderer_threads, ct.c_uint64(69))
        print("Got value {}".format(r.get_num(cr_renderer_param.cr_renderer_threads)))
